// OpenSTA, Static Timing Analyzer
// Copyright (c) 2026, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
// 
// The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software.
// 
// Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// This notice may not be removed or altered from any source distribution.

#include "StaDb.hh"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "ConcreteLibrary.hh"
#include "ConcreteNetwork.hh"
#include "DbCodec.hh"
#include "DbFile.hh"
#include "DbGraph.hh"
#include "DbSdc.hh"
#include "DbSearch.hh"
#include "DbSections.hh"
#include "Debug.hh"
#include "Format.hh"
#include "FuncExpr.hh"
#include "InternalPower.hh"
#include "LeakagePower.hh"
#include "Liberty.hh"
#include "LinearModel.hh"
#include "Network.hh"
#include "PatternMatch.hh"
#include "PortDirection.hh"
#include "Scene.hh"
#include "Search.hh"
#include "Sequential.hh"
#include "Sta.hh"
#include "Stats.hh"
#include "TableModel.hh"
#include "TimingArc.hh"
#include "TimingRole.hh"
#include "Units.hh"

namespace sta {

DbUnsupported::DbUnsupported(const std::string &msg) :
  msg_(sta::format("{} {}", stadb_error_unsupported, msg))
{
}

const char *
DbUnsupported::what() const noexcept
{
  return msg_.c_str();
}

////////////////////////////////////////////////////////////////

// Encodes the liberty libraries. Axes and tables are shared by pointer between
// models, so they are pooled and referenced by id rather than written inline;
// that preserves the sharing on read and keeps the section from exploding on
// libraries where every arc reuses one template axis.
class DbLibertyWriter
{
public:
  DbLibertyWriter(DbWriter &writer, Sta *sta);
  void write(const std::vector<LibertyLibrary*> &libraries);

private:
  void collectLibrary(LibertyLibrary *library);
  void collectCell(LibertyCell *cell);
  void collectModel(const TimingModel *model);
  void collectModels(const TableModels *models);
  DbPoolId collectTableModel(const TableModel *model);
  DbPoolId collectTable(const Table *table);
  DbPoolId collectAxis(const TableAxis *axis);

  void writePools();
  void writeLibrary(LibertyLibrary *library);
  void writeCell(LibertyCell *cell);
  void writePortStructure(const LibertyPort *port);
  void writePortAttrs(const LibertyPort *port);
  void writeArcSet(const TimingArcSet *arc_set);
  void writeModel(const TimingModel *model);
  void writeModels(const TableModels *models);
  void writeTableModel(const TableModel *model);
  void writeInternalPower(const InternalPower &pwr);
  void writeLeakagePower(const LeakagePower &pwr);
  void writeStatetable(const Statetable *statetable);
  void writeModeDefs(const LibertyCell *cell);
  void writeOcvDerate(OcvDerate &derate);
  void writeOcvDerates(OcvDerateMap &derates);
  void writeFuncExpr(const FuncExpr *expr);
  void writeUnit(const Unit *unit);

  void collectOcvDerate(OcvDerate &derate);

  // Canonical port order shared with the reader: cell ports in declaration
  // order, each container followed by its members.
  void indexPorts(LibertyCell *cell);
  uint32_t portId(const LibertyPort *port) const;

  DbWriter &writer_;
  Sta *sta_;
  std::vector<const TableAxis*> axes_;
  std::unordered_map<const TableAxis*, DbPoolId> axis_ids_;
  std::vector<const Table*> tables_;
  std::unordered_map<const Table*, DbPoolId> table_ids_;
  // Rebuilt per cell.
  std::unordered_map<const LibertyPort*, uint32_t> port_ids_;
  bool ccs_dropped_{false};
};

DbLibertyWriter::DbLibertyWriter(DbWriter &writer, Sta *sta) :
  writer_(writer),
  sta_(sta)
{
}

DbPoolId
DbLibertyWriter::collectAxis(const TableAxis *axis)
{
  if (axis == nullptr)
    return 0;
  auto itr = axis_ids_.find(axis);
  if (itr != axis_ids_.end())
    return itr->second;
  axes_.push_back(axis);
  DbPoolId id = static_cast<DbPoolId>(axes_.size());
  axis_ids_[axis] = id;
  return id;
}

DbPoolId
DbLibertyWriter::collectTable(const Table *table)
{
  if (table == nullptr)
    return 0;
  auto itr = table_ids_.find(table);
  if (itr != table_ids_.end())
    return itr->second;
  collectAxis(table->axis1());
  collectAxis(table->axis2());
  collectAxis(table->axis3());
  tables_.push_back(table);
  DbPoolId id = static_cast<DbPoolId>(tables_.size());
  table_ids_[table] = id;
  return id;
}

DbPoolId
DbLibertyWriter::collectTableModel(const TableModel *model)
{
  if (model == nullptr)
    return 0;
  return collectTable(model->table().get());
}

void
DbLibertyWriter::collectModels(const TableModels *models)
{
  if (models == nullptr)
    return;
  collectTableModel(models->model());
  collectTableModel(models->sigma(EarlyLate::early()));
  collectTableModel(models->sigma(EarlyLate::late()));
  collectTableModel(models->stdDev());
  collectTableModel(models->meanShift());
  collectTableModel(models->skewness());
}

void
DbLibertyWriter::collectModel(const TimingModel *model)
{
  if (model == nullptr)
    return;
  const GateTableModel *gate = dynamic_cast<const GateTableModel*>(model);
  if (gate) {
    // CCS data has no public read accessor on ReceiverModel, so it cannot be
    // round tripped. Dropping it is safe for the NLDM analysis this format
    // targets, since a cell carrying CCS carries the equivalent NLDM tables
    // alongside it, but say so once rather than losing it quietly.
    if (gate->receiverModel() || gate->outputWaveforms())
      ccs_dropped_ = true;
    collectModels(gate->delayModels());
    collectModels(gate->slewModels());
    return;
  }
  const CheckTableModel *check = dynamic_cast<const CheckTableModel*>(model);
  if (check) {
    collectModels(check->checkModels());
    return;
  }
  // Linear models carry only scalars, so nothing to pool.
}

void
DbLibertyWriter::collectOcvDerate(OcvDerate &derate)
{
  for (const RiseFall *rf : RiseFall::range()) {
    for (const EarlyLate *early_late : EarlyLate::range()) {
      for (int path = 0; path < path_type_count; path++)
        collectTable(derate.derateTable(rf, early_late,
                                        static_cast<PathType>(path)));
    }
  }
}

void
DbLibertyWriter::collectCell(LibertyCell *cell)
{
  for (const TimingArcSet *arc_set : cell->timingArcSets()) {
    for (const TimingArc *arc : arc_set->arcs())
      collectModel(arc->model());
  }
  for (const InternalPower &pwr : cell->internalPowers()) {
    for (const RiseFall *rf : RiseFall::range())
      collectTableModel(pwr.model(rf).model());
  }
  for (auto &[name, derate] : cell->ocv_derate_map_) {
    (void)name;
    collectOcvDerate(derate);
  }
  if (cell->testCell())
    collectCell(cell->testCell());
}

void
DbLibertyWriter::collectLibrary(LibertyLibrary *library)
{
  if (library->delayModelType() != DelayModelType::table)
    throw DbUnsupported(sta::format("liberty library {} uses a non table delay "
                                    "model", library->name()));
  for (int i = 0; i < table_template_type_count; i++) {
    TableTemplateType type = static_cast<TableTemplateType>(i);
    for (TableTemplate *tmpl : library->tableTemplates(type)) {
      collectAxis(tmpl->axis1());
      collectAxis(tmpl->axis2());
      collectAxis(tmpl->axis3());
    }
  }
  for (auto &[name, derate] : library->ocv_derate_map_) {
    (void)name;
    collectOcvDerate(derate);
  }
  LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext())
    collectCell(cell_iter.next());
}

////////////////////////////////////////////////////////////////

void
DbLibertyWriter::writePools()
{
  writer_.putU32(static_cast<uint32_t>(axes_.size()));
  for (const TableAxis *axis : axes_) {
    writer_.putU8(static_cast<uint8_t>(axis->variable()));
    const FloatSeq &values = axis->values();
    writer_.putU32(static_cast<uint32_t>(values.size()));
    for (float value : values)
      writer_.putF32(value);
  }

  writer_.putU32(static_cast<uint32_t>(tables_.size()));
  for (const Table *table : tables_) {
    int order = table->order();
    writer_.putU8(static_cast<uint8_t>(order));
    writer_.putU32(axis_ids_.count(table->axis1()) ? axis_ids_[table->axis1()] : 0);
    writer_.putU32(axis_ids_.count(table->axis2()) ? axis_ids_[table->axis2()] : 0);
    writer_.putU32(axis_ids_.count(table->axis3()) ? axis_ids_[table->axis3()] : 0);
    if (order == 0)
      writer_.putF32(table->value(0, 0, 0));
    else if (order == 1) {
      const FloatSeq *values = table->values();
      writer_.putU32(static_cast<uint32_t>(values->size()));
      for (float value : *values)
        writer_.putF32(value);
    }
    else {
      const FloatTable *values = table->values3();
      writer_.putU32(static_cast<uint32_t>(values->size()));
      for (const FloatSeq &row : *values) {
        writer_.putU32(static_cast<uint32_t>(row.size()));
        for (float value : row)
          writer_.putF32(value);
      }
    }
  }
}

void
DbLibertyWriter::writeTableModel(const TableModel *model)
{
  DbTableModelRec rec;
  rec.table = table_ids_.count(model->table().get())
    ? table_ids_[model->table().get()] : 0;
  const TableTemplate *tmpl = model->tblTemplate();
  rec.tbl_template = writer_.strings()->intern(tmpl ? tmpl->name() : "");
  rec.template_type = tmpl ? static_cast<uint8_t>(tmpl->type()) : 0;
  rec.scale_factor_type = static_cast<uint8_t>(model->scaleFactorType());
  rec.rf_index = static_cast<uint8_t>(model->rfIndex());
  visit(writer_, rec);
}

void
DbLibertyWriter::writeInternalPower(const InternalPower &pwr)
{
  writer_.putU32(portId(pwr.port()));
  writer_.putU32(portId(pwr.relatedPort()));
  writer_.putU32(portId(pwr.relatedPgPin()));
  writeFuncExpr(pwr.when());
  const TableModel *rise = pwr.model(RiseFall::rise()).model();
  const TableModel *fall = pwr.model(RiseFall::fall()).model();
  uint8_t present = 0;
  if (rise)
    present |= stadb_internal_power_rise;
  if (fall)
    present |= stadb_internal_power_fall;
  if (rise && fall && rise == fall)
    present |= stadb_internal_power_aliased;
  writer_.putU8(present);
  if (present & stadb_internal_power_rise)
    writeTableModel(rise);
  if ((present & stadb_internal_power_fall)
      && !(present & stadb_internal_power_aliased))
    writeTableModel(fall);
}

void
DbLibertyWriter::writeLeakagePower(const LeakagePower &pwr)
{
  writer_.putU32(portId(pwr.relatedPgPort()));
  writeFuncExpr(pwr.when());
  writer_.putF32(pwr.power());
}

void
DbLibertyWriter::writeStatetable(const Statetable *statetable)
{
  const LibertyPortSeq &inputs = statetable->inputPorts();
  writer_.putU32(static_cast<uint32_t>(inputs.size()));
  for (const LibertyPort *lib_port : inputs)
    writer_.putU32(portId(lib_port));
  const LibertyPortSeq &internals = statetable->internalPorts();
  writer_.putU32(static_cast<uint32_t>(internals.size()));
  for (const LibertyPort *lib_port : internals)
    writer_.putU32(portId(lib_port));
  const StatetableRows &rows = statetable->table();
  writer_.putU32(static_cast<uint32_t>(rows.size()));
  for (const StatetableRow &row : rows) {
    const StateInputValues &inputs_vals = row.inputValues();
    writer_.putU32(static_cast<uint32_t>(inputs_vals.size()));
    for (StateInputValue value : inputs_vals)
      writer_.putU8(static_cast<uint8_t>(value));
    const StateInternalValues &current_vals = row.currentValues();
    writer_.putU32(static_cast<uint32_t>(current_vals.size()));
    for (StateInternalValue value : current_vals)
      writer_.putU8(static_cast<uint8_t>(value));
    const StateInternalValues &next_vals = row.nextValues();
    writer_.putU32(static_cast<uint32_t>(next_vals.size()));
    for (StateInternalValue value : next_vals)
      writer_.putU8(static_cast<uint8_t>(value));
  }
}

void
DbLibertyWriter::writeModeDefs(const LibertyCell *cell)
{
  const ModeDefMap &mode_defs = cell->mode_defs_;
  writer_.putU32(static_cast<uint32_t>(mode_defs.size()));
  for (const auto &[name, mode_def] : mode_defs) {
    writer_.putStr(name);
    const ModeValueMap &values = mode_def.values();
    writer_.putU32(static_cast<uint32_t>(values.size()));
    for (const auto &[value_name, value_def] : values) {
      writer_.putStr(value_name);
      writer_.putStr(value_def.sdfCond());
      writeFuncExpr(value_def.cond());
    }
  }
}

void
DbLibertyWriter::writeOcvDerate(OcvDerate &derate)
{
  writer_.putStr(derate.name());
  for (const RiseFall *rf : RiseFall::range()) {
    for (const EarlyLate *early_late : EarlyLate::range()) {
      for (int path = 0; path < path_type_count; path++) {
        const Table *table = derate.derateTable(rf, early_late,
                                               static_cast<PathType>(path));
        writer_.putU32(table && table_ids_.count(table)
                       ? table_ids_[table] : 0);
      }
    }
  }
}

void
DbLibertyWriter::writeOcvDerates(OcvDerateMap &derates)
{
  writer_.putU32(static_cast<uint32_t>(derates.size()));
  for (auto &[name, derate] : derates) {
    (void)name;
    writeOcvDerate(derate);
  }
}

void
DbLibertyWriter::writeModels(const TableModels *models)
{
  TableModel *sigma_early = models->sigma(EarlyLate::early());
  TableModel *sigma_late = models->sigma(EarlyLate::late());
  uint8_t present = 0;
  if (models->model())
    present |= stadb_models_model;
  if (sigma_early)
    present |= stadb_models_sigma_early;
  if (sigma_late && sigma_late != sigma_early)
    present |= stadb_models_sigma_late;
  if (sigma_early && sigma_early == sigma_late)
    present |= stadb_models_sigma_aliased;
  if (models->stdDev())
    present |= stadb_models_std_dev;
  if (models->meanShift())
    present |= stadb_models_mean_shift;
  if (models->skewness())
    present |= stadb_models_skewness;
  writer_.putU8(present);
  if (present & stadb_models_model)
    writeTableModel(models->model());
  if (present & stadb_models_sigma_early)
    writeTableModel(sigma_early);
  if (present & stadb_models_sigma_late)
    writeTableModel(sigma_late);
  if (present & stadb_models_std_dev)
    writeTableModel(models->stdDev());
  if (present & stadb_models_mean_shift)
    writeTableModel(models->meanShift());
  if (present & stadb_models_skewness)
    writeTableModel(models->skewness());
}

void
DbLibertyWriter::writeModel(const TimingModel *model)
{
  if (model == nullptr) {
    writer_.putU8(static_cast<uint8_t>(DbModelKind::none));
    return;
  }
  const GateTableModel *gate = dynamic_cast<const GateTableModel*>(model);
  if (gate) {
    writer_.putU8(static_cast<uint8_t>(DbModelKind::gate_table));
    writer_.putBool(gate->delayModels() != nullptr);
    if (gate->delayModels())
      writeModels(gate->delayModels());
    writer_.putBool(gate->slewModels() != nullptr);
    if (gate->slewModels())
      writeModels(gate->slewModels());
    return;
  }
  const CheckTableModel *check = dynamic_cast<const CheckTableModel*>(model);
  if (check) {
    writer_.putU8(static_cast<uint8_t>(DbModelKind::check_table));
    writer_.putBool(check->checkModels() != nullptr);
    if (check->checkModels())
      writeModels(check->checkModels());
    return;
  }
  // Linear models only exist for cmos_linear libraries, which collectLibrary
  // already rejects, and their scalars have no public accessors to read back.
  throw DbUnsupported("timing arc has a non table timing model");
}

////////////////////////////////////////////////////////////////

void
DbLibertyWriter::writeFuncExpr(const FuncExpr *expr)
{
  if (expr == nullptr) {
    writer_.putU8(static_cast<uint8_t>(DbFuncKind::null));
    return;
  }
  switch (expr->op()) {
  case FuncExpr::Op::port:
    writer_.putU8(static_cast<uint8_t>(DbFuncKind::port));
    writer_.putU32(portId(expr->port()));
    break;
  case FuncExpr::Op::not_:
    writer_.putU8(static_cast<uint8_t>(DbFuncKind::not_));
    writeFuncExpr(expr->left());
    break;
  case FuncExpr::Op::or_:
  case FuncExpr::Op::and_:
  case FuncExpr::Op::xor_: {
    DbFuncKind kind = expr->op() == FuncExpr::Op::or_ ? DbFuncKind::or_
      : (expr->op() == FuncExpr::Op::and_ ? DbFuncKind::and_ : DbFuncKind::xor_);
    writer_.putU8(static_cast<uint8_t>(kind));
    writeFuncExpr(expr->left());
    writeFuncExpr(expr->right());
    break;
  }
  case FuncExpr::Op::one:
    writer_.putU8(static_cast<uint8_t>(DbFuncKind::one));
    break;
  case FuncExpr::Op::zero:
    writer_.putU8(static_cast<uint8_t>(DbFuncKind::zero));
    break;
  }
}

void
DbLibertyWriter::writeUnit(const Unit *unit)
{
  writer_.putF32(unit->scale());
  writer_.putStr(unit->suffix());
  writer_.putI32(unit->digits());
}

////////////////////////////////////////////////////////////////

void
DbLibertyWriter::indexPorts(LibertyCell *cell)
{
  port_ids_.clear();
  uint32_t id = 1;
  LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    LibertyPort *port = port_iter.next();
    port_ids_[port] = id++;
    if (port->hasMembers()) {
      LibertyPortMemberIterator member_iter(port);
      while (member_iter.hasNext())
        port_ids_[member_iter.next()] = id++;
    }
  }
}

uint32_t
DbLibertyWriter::portId(const LibertyPort *port) const
{
  if (port == nullptr)
    return 0;
  auto itr = port_ids_.find(port);
  return itr == port_ids_.end() ? 0 : itr->second;
}

void
DbLibertyWriter::writePortStructure(const LibertyPort *port)
{
  writer_.putStr(port->name());
  if (port->isBus()) {
    writer_.putU8(static_cast<uint8_t>(DbPortKind::bus));
    writer_.putI32(port->fromIndex());
    writer_.putI32(port->toIndex());
    const BusDcl *bus_dcl = port->busDcl();
    writer_.putStr(bus_dcl ? bus_dcl->name() : "");
  }
  else if (port->isBundle()) {
    writer_.putU8(static_cast<uint8_t>(DbPortKind::bundle));
    LibertyPortMemberIterator member_iter(port);
    std::vector<const LibertyPort*> members;
    while (member_iter.hasNext())
      members.push_back(member_iter.next());
    writer_.putU32(static_cast<uint32_t>(members.size()));
    for (const LibertyPort *member : members)
      writer_.putStr(member->name());
  }
  else
    writer_.putU8(static_cast<uint8_t>(DbPortKind::scalar));
}

void
DbLibertyWriter::writePortAttrs(const LibertyPort *port)
{
  DbPortRec rec;
  PortDirection *dir = port->direction();
  rec.direction = writer_.strings()->intern(dir ? dir->name() : "");
  rec.pwr_gnd_type = static_cast<uint8_t>(port->pwrGndType());
  rec.voltage_name = writer_.strings()->intern(port->voltageName());
  rec.scan_signal_type = static_cast<uint8_t>(port->scanSignalType());
  port->fanoutLoad(rec.fanout_load, rec.fanout_load_exists);
  bool min_period_exists;
  port->minPeriod(rec.min_period, min_period_exists);
  rec.is_clk = port->isClock();
  rec.is_reg_clk = port->isRegClk();
  rec.is_check_clk = port->isCheckClk();
  rec.is_reg_output = port->isRegOutput();
  rec.is_latch_data = port->isLatchData();
  rec.is_latch_output = port->isLatchOutput();
  rec.is_clk_gate_clk = port->isClockGateClock();
  rec.is_clk_gate_enable = port->isClockGateEnable();
  rec.is_clk_gate_out = port->isClockGateOut();
  rec.is_pll_feedback = port->isPllFeedback();
  rec.is_switch = port->isSwitch();
  rec.is_pad = port->isPad();
  rec.isolation_cell_data = port->isolationCellData();
  rec.isolation_cell_enable = port->isolationCellEnable();
  rec.level_shifter_data = port->levelShifterData();
  const RiseFall *trigger = port->pulseClkTrigger();
  const RiseFall *sense = port->pulseClkSense();
  // Zero means absent, so transitions are stored biased by one.
  rec.pulse_clk_trigger = trigger ? trigger->index() + 1 : 0;
  rec.pulse_clk_sense = sense ? sense->index() + 1 : 0;
  visit(writer_, rec);

  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *mm : MinMax::range()) {
      float cap;
      bool exists;
      port->capacitance(rf, mm, cap, exists);
      writer_.putBool(exists);
      if (exists)
        writer_.putF32(cap);
    }
  }
  for (const MinMax *mm : MinMax::range()) {
    float limit;
    bool exists;
    port->slewLimit(mm, limit, exists);
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(limit);
    port->capacitanceLimit(mm, limit, exists);
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(limit);
    port->fanoutLimit(mm, limit, exists);
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(limit);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    float width;
    bool exists;
    port->minPulseWidth(rf, width, exists);
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(width);
  }
  writeFuncExpr(port->function());
  writeFuncExpr(port->tristateEnable());
  writer_.putU32(portId(port->relatedGroundPort()));
  writer_.putU32(portId(port->relatedPowerPort()));
}

void
DbLibertyWriter::writeArcSet(const TimingArcSet *arc_set)
{
  DbArcSetRec rec;
  rec.from_port = portId(arc_set->from());
  rec.to_port = portId(arc_set->to());
  rec.related_out_port = portId(arc_set->relatedOut());
  rec.role = writer_.strings()->intern(arc_set->role()->to_string());
  rec.timing_type = writer_.strings()->intern(to_string(arc_set->timingType()));
  rec.timing_sense = static_cast<uint8_t>(arc_set->sense());
  rec.sdf_cond = writer_.strings()->intern(arc_set->sdfCond());
  rec.sdf_cond_start = writer_.strings()->intern(arc_set->sdfCondStart());
  rec.sdf_cond_end = writer_.strings()->intern(arc_set->sdfCondEnd());
  rec.mode_name = writer_.strings()->intern(arc_set->modeName());
  rec.mode_value = writer_.strings()->intern(arc_set->modeValue());
  rec.ocv_arc_depth = arc_set->ocvArcDepth();
  visit(writer_, rec);
  writeFuncExpr(arc_set->cond());

  const TimingArcSeq &arcs = arc_set->arcs();
  writer_.putU32(static_cast<uint32_t>(arcs.size()));
  for (const TimingArc *arc : arcs) {
    // By name rather than index so that tristate transitions round trip and an
    // upstream index change cannot silently remap them.
    writer_.putStr(arc->fromEdge()->to_string());
    writer_.putStr(arc->toEdge()->to_string());
    writeModel(arc->model());
  }
}

void
DbLibertyWriter::writeCell(LibertyCell *cell)
{
  indexPorts(cell);

  DbCellRec rec;
  rec.name = writer_.strings()->intern(cell->name());
  rec.filename = writer_.strings()->intern(cell->filename());
  rec.area = cell->area();
  rec.dont_use = cell->dontUse();
  rec.is_macro = cell->isMacro();
  rec.is_memory = cell->isMemory();
  rec.is_pad = cell->isPad();
  rec.is_clock_cell = cell->isClockCell();
  rec.is_level_shifter = cell->isLevelShifter();
  rec.level_shifter_type = static_cast<uint8_t>(cell->levelShifterType());
  rec.is_isolation_cell = cell->isIsolationCell();
  rec.always_on = cell->alwaysOn();
  rec.switch_cell_type = static_cast<uint8_t>(cell->switchCellType());
  rec.interface_timing = cell->interfaceTiming();
  // There is no getter for the enum, so recover it from the predicates.
  ClockGateType clock_gate_type = ClockGateType::none;
  if (cell->isClockGateLatchPosedge())
    clock_gate_type = ClockGateType::latch_posedge;
  else if (cell->isClockGateLatchNegedge())
    clock_gate_type = ClockGateType::latch_negedge;
  else if (cell->isClockGateOther())
    clock_gate_type = ClockGateType::other;
  rec.clock_gate_type = static_cast<uint8_t>(clock_gate_type);
  rec.has_infered_reg_timing_arcs = cell->hasInferedRegTimingArcs();
  rec.ocv_arc_depth = cell->ocvArcDepth();
  rec.footprint = writer_.strings()->intern(cell->footprint());
  rec.user_function_class = writer_.strings()->intern(cell->userFunctionClass());
  cell->leakagePower(rec.leakage_power, rec.leakage_power_exists);
  visit(writer_, rec);

  // Port structure first so the reader can create every port before any
  // function expression or timing arc refers to one by id.
  std::vector<LibertyPort*> containers;
  LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext())
    containers.push_back(port_iter.next());
  writer_.putU32(static_cast<uint32_t>(containers.size()));
  for (const LibertyPort *port : containers)
    writePortStructure(port);

  writer_.putU32(static_cast<uint32_t>(port_ids_.size()));
  for (LibertyPort *port : containers) {
    writePortAttrs(port);
    if (port->hasMembers()) {
      LibertyPortMemberIterator member_iter(port);
      while (member_iter.hasNext())
        writePortAttrs(member_iter.next());
    }
  }

  const SequentialSeq &sequentials = cell->sequentials();
  writer_.putU32(static_cast<uint32_t>(sequentials.size()));
  for (const Sequential &seq : sequentials) {
    writer_.putBool(seq.isRegister());
    writeFuncExpr(seq.clock());
    writeFuncExpr(seq.data());
    writeFuncExpr(seq.clear());
    writeFuncExpr(seq.preset());
    writer_.putU8(static_cast<uint8_t>(seq.clearPresetOutput()));
    writer_.putU8(static_cast<uint8_t>(seq.clearPresetOutputInv()));
    writer_.putU32(portId(seq.output()));
    writer_.putU32(portId(seq.outputInv()));
  }

  // LibertyReader builds test_cell before timing/statetable so its ports are
  // visible to those groups; keep the same order here.
  TestCell *test_cell = cell->testCell();
  writer_.putBool(test_cell != nullptr);
  if (test_cell) {
    std::unordered_map<const LibertyPort*, uint32_t> saved_port_ids = port_ids_;
    writeCell(test_cell);
    port_ids_ = std::move(saved_port_ids);
  }

  const TimingArcSetSeq &arc_sets = cell->timingArcSets();
  writer_.putU32(static_cast<uint32_t>(arc_sets.size()));
  for (const TimingArcSet *arc_set : arc_sets)
    writeArcSet(arc_set);

  const InternalPowerSeq &internal_powers = cell->internalPowers();
  writer_.putU32(static_cast<uint32_t>(internal_powers.size()));
  for (const InternalPower &pwr : internal_powers)
    writeInternalPower(pwr);

  const LeakagePowerSeq &leakage_powers = cell->leakagePowers();
  writer_.putU32(static_cast<uint32_t>(leakage_powers.size()));
  for (const LeakagePower &pwr : leakage_powers)
    writeLeakagePower(pwr);

  const Statetable *statetable = cell->statetable();
  writer_.putBool(statetable != nullptr);
  if (statetable)
    writeStatetable(statetable);

  writeModeDefs(cell);
  writeOcvDerates(cell->ocv_derate_map_);
  OcvDerate *ocv_group = cell->ocv_derate_;
  writer_.putStr(ocv_group ? ocv_group->name() : "");
}

void
DbLibertyWriter::writeLibrary(LibertyLibrary *library)
{
  writer_.putStr(library->name());
  writer_.putStr(library->filename());
  writer_.putU8(static_cast<uint8_t>(library->delayModelType()));

  const Units *units = library->units();
  writeUnit(units->timeUnit());
  writeUnit(units->capacitanceUnit());
  writeUnit(units->voltageUnit());
  writeUnit(units->resistanceUnit());
  writeUnit(units->currentUnit());
  writeUnit(units->powerUnit());
  writeUnit(units->distanceUnit());

  writer_.putF32(library->nominalProcess());
  writer_.putF32(library->nominalVoltage());
  writer_.putF32(library->nominalTemperature());
  writer_.putF32(library->defaultInputPinCap());
  writer_.putF32(library->defaultOutputPinCap());
  writer_.putF32(library->defaultBidirectPinCap());
  // These four carry an exists flag rather than a sentinel value.
  float value;
  bool exists;
  library->defaultFanoutLoad(value, exists);
  writer_.putBool(exists);
  writer_.putF32(value);
  library->defaultMaxSlew(value, exists);
  writer_.putBool(exists);
  writer_.putF32(value);
  library->defaultMaxCapacitance(value, exists);
  writer_.putBool(exists);
  writer_.putF32(value);
  library->defaultMaxFanout(value, exists);
  writer_.putBool(exists);
  writer_.putF32(value);
  writer_.putF32(library->slewDerateFromLibrary());
  writer_.putF32(library->ocvArcDepth());
  for (const RiseFall *rf : RiseFall::range()) {
    writer_.putF32(library->inputThreshold(rf));
    writer_.putF32(library->outputThreshold(rf));
    writer_.putF32(library->slewLowerThreshold(rf));
    writer_.putF32(library->slewUpperThreshold(rf));
  }

  // Only the default operating conditions are reachable through the public
  // API, and they are the ones delay calculation falls back to when the SDC
  // sets none.
  const OperatingConditions *op_cond = library->defaultOperatingConditions();
  writer_.putBool(op_cond != nullptr);
  if (op_cond) {
    writer_.putStr(op_cond->name());
    writer_.putF32(op_cond->process());
    writer_.putF32(op_cond->voltage());
    writer_.putF32(op_cond->temperature());
    writer_.putU8(static_cast<uint8_t>(op_cond->wireloadTree()));
  }

  // The scale array is dense and enum ordered, so the counts are written and
  // checked on read to catch an upstream enumerator being added.
  ScaleFactors *scales = library->scaleFactors();
  writer_.putBool(scales != nullptr);
  if (scales) {
    writer_.putStr(scales->name());
    writer_.putU8(scale_factor_type_count);
    writer_.putU8(scale_factor_pvt_count);
    for (int type = 0; type < scale_factor_type_count; type++) {
      for (int pvt = 0; pvt < scale_factor_pvt_count; pvt++) {
        for (const RiseFall *rf : RiseFall::range())
          writer_.putF32(scales->scale(static_cast<ScaleFactorType>(type),
                                       static_cast<ScaleFactorPvt>(pvt), rf));
      }
    }
  }

  // Table templates reference pooled axes by id.
  for (int i = 0; i < table_template_type_count; i++) {
    TableTemplateType type = static_cast<TableTemplateType>(i);
    const std::vector<TableTemplate*> &templates = library->tableTemplates(type);
    writer_.putU32(static_cast<uint32_t>(templates.size()));
    for (const TableTemplate *tmpl : templates) {
      writer_.putStr(tmpl->name());
      writer_.putU32(collectAxis(tmpl->axis1()));
      writer_.putU32(collectAxis(tmpl->axis2()));
      writer_.putU32(collectAxis(tmpl->axis3()));
    }
  }

  writeOcvDerates(library->ocv_derate_map_);
  OcvDerate *default_ocv = library->default_ocv_derate_;
  writer_.putStr(default_ocv ? default_ocv->name() : "");

  std::vector<LibertyCell*> cells;
  LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext())
    cells.push_back(cell_iter.next());
  writer_.putU32(static_cast<uint32_t>(cells.size()));
  for (LibertyCell *cell : cells)
    writeCell(cell);
}

void
DbLibertyWriter::write(const std::vector<LibertyLibrary*> &libraries)
{
  for (LibertyLibrary *library : libraries)
    collectLibrary(library);
  if (ccs_dropped_)
    sta_->report()->warn(2741, "CCS receiver capacitance and output current "
                         "data is not saved to the stadb; the restored session "
                         "uses the NLDM tables only.");
  writePools();
  writer_.putU32(static_cast<uint32_t>(libraries.size()));
  for (LibertyLibrary *library : libraries)
    writeLibrary(library);
}

////////////////////////////////////////////////////////////////

// Encodes the linked network: the cell masters that liberty does not supply
// (verilog modules), then the instance tree with its nets, pins and terms.
class DbNetworkWriter
{
public:
  DbNetworkWriter(DbWriter &writer, ConcreteNetwork *network);
  void write();

private:
  void collect();
  void collectInstance(Instance *inst);
  uint32_t cellRef(const Cell *cell);

  void writeLibraries();
  void writeCell(const Cell *cell);
  void writePort(const Port *port);
  void writeInstances();
  void writeNets();
  void writePins();

  DbWriter &writer_;
  ConcreteNetwork *network_;
  std::vector<Instance*> instances_;
  std::unordered_map<const Instance*, DbNetworkId> instance_ids_;
  std::vector<Net*> nets_;
  std::unordered_map<const Net*, DbNetworkId> net_ids_;
  std::vector<Pin*> pins_;
  // Pins carrying a term, in term creation order.
  std::vector<DbNetworkId> term_pins_;
  // Cells named by instances, as (library name, cell name) pairs the reader
  // looks up rather than defines, since liberty cells come from that section.
  std::vector<const Cell*> cells_;
  std::unordered_map<const Cell*, uint32_t> cell_ids_;
};

DbNetworkWriter::DbNetworkWriter(DbWriter &writer, ConcreteNetwork *network) :
  writer_(writer),
  network_(network)
{
}

uint32_t
DbNetworkWriter::cellRef(const Cell *cell)
{
  auto itr = cell_ids_.find(cell);
  if (itr != cell_ids_.end())
    return itr->second;
  uint32_t id = static_cast<uint32_t>(cells_.size());
  cells_.push_back(cell);
  cell_ids_[cell] = id;
  return id;
}

void
DbNetworkWriter::collectInstance(Instance *inst)
{
  instances_.push_back(inst);
  cellRef(network_->cell(inst));

  // Includes merged nets, which netIterator hides. They survive as name
  // aliases that findNet resolves, so dropping them would silently break any
  // constraint that names the merged-away side of an assign.
  PatternMatch all("*");
  NetSeq inst_nets;
  network_->findInstNetsMatching(inst, &all, inst_nets);
  for (const Net *net : inst_nets)
    nets_.push_back(const_cast<Net*>(net));

  InstancePinIterator *pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext())
    pins_.push_back(pin_iter->next());
  delete pin_iter;

  InstanceChildIterator *child_iter = network_->childIterator(inst);
  while (child_iter->hasNext())
    collectInstance(child_iter->next());
  delete child_iter;
}

void
DbNetworkWriter::collect()
{
  collectInstance(network_->topInstance());

  auto by_id = [&](const auto *a, const auto *b) {
    return network_->id(a) < network_->id(b);
  };
  std::sort(instances_.begin(), instances_.end(), by_id);
  std::sort(nets_.begin(), nets_.end(), by_id);
  std::sort(pins_.begin(), pins_.end(), by_id);

  for (size_t i = 0; i < instances_.size(); i++)
    instance_ids_[instances_[i]] = static_cast<DbNetworkId>(i);
  for (size_t i = 0; i < nets_.size(); i++)
    net_ids_[nets_[i]] = static_cast<DbNetworkId>(i);

  // Terms get their own order because they are not created with their pin.
  // connect() makes a term the first time a net reaches the top instance,
  // which can be long after the pin, so term ids do not follow pin ids. Net
  // term lists are prepend-built, so replaying them in the wrong order would
  // reverse them: correct timing, but a netlist that no longer round trips.
  for (size_t i = 0; i < pins_.size(); i++) {
    if (network_->term(pins_[i]))
      term_pins_.push_back(static_cast<DbNetworkId>(i));
  }
  std::sort(term_pins_.begin(), term_pins_.end(),
            [&](DbNetworkId a, DbNetworkId b) {
              return network_->id(network_->term(pins_[a]))
                < network_->id(network_->term(pins_[b]));
            });
}

void
DbNetworkWriter::writePort(const Port *port)
{
  DbPortKind kind = DbPortKind::scalar;
  if (network_->isBus(port))
    kind = DbPortKind::bus;
  else if (network_->isBundle(port))
    kind = DbPortKind::bundle;
  writer_.putStr(network_->name(port));
  writer_.putU8(static_cast<uint8_t>(kind));
  writer_.putStr(network_->direction(port)->name());
  if (kind == DbPortKind::bus) {
    writer_.putI32(network_->fromIndex(port));
    writer_.putI32(network_->toIndex(port));
  }
  else if (kind == DbPortKind::bundle) {
    PortSeq members;
    PortMemberIterator *member_iter = network_->memberIterator(port);
    while (member_iter->hasNext())
      members.push_back(member_iter->next());
    delete member_iter;
    writer_.putU32(static_cast<uint32_t>(members.size()));
    for (const Port *member : members)
      writer_.putStr(network_->name(member));
  }

  // setDirection propagates from a container to its members, so only the
  // container direction is stored. A member that disagrees would be lost.
  if (kind != DbPortKind::scalar) {
    PortMemberIterator *member_iter = network_->memberIterator(port);
    while (member_iter->hasNext()) {
      const Port *member = member_iter->next();
      if (member && network_->direction(member) != network_->direction(port)) {
        delete member_iter;
        throw DbUnsupported(sta::format("port {} has a member whose direction "
                                        "differs from the bus or bundle",
                                        network_->name(port)));
      }
    }
    delete member_iter;
  }
}

void
DbNetworkWriter::writeCell(const Cell *cell)
{
  writer_.putStr(network_->name(cell));
  writer_.putStr(network_->filename(cell));
  writer_.putBool(network_->isLeaf(cell));

  const AttributeMap &attrs = network_->attributeMap(cell);
  writer_.putU32(static_cast<uint32_t>(attrs.size()));
  for (const auto &[key, value] : attrs) {
    writer_.putStr(key);
    writer_.putStr(value);
  }

  // Bundle members must exist before the bundle names them, and bus bits are
  // created by the bus port itself, so declaration order is also a valid
  // rebuild order.
  CellPortIterator *port_iter = network_->portIterator(cell);
  PortSeq ports;
  while (port_iter->hasNext())
    ports.push_back(port_iter->next());
  delete port_iter;
  writer_.putU32(static_cast<uint32_t>(ports.size()));
  for (const Port *port : ports)
    writePort(port);
}

void
DbNetworkWriter::writeLibraries()
{
  // Liberty libraries are restored by the liberty section; only the masters
  // that nothing else recreates, such as verilog modules, belong here.
  std::vector<ConcreteLibrary*> libraries;
  LibraryIterator *lib_iter = network_->libraryIterator();
  while (lib_iter->hasNext()) {
    ConcreteLibrary *library =
      reinterpret_cast<ConcreteLibrary*>(lib_iter->next());
    if (!library->isLiberty())
      libraries.push_back(library);
  }
  delete lib_iter;

  writer_.putU32(static_cast<uint32_t>(libraries.size()));
  for (ConcreteLibrary *library : libraries) {
    writer_.putStr(library->name());
    writer_.putStr(library->filename());
    writer_.putU8(static_cast<uint8_t>(library->busBrktLeft()));
    writer_.putU8(static_cast<uint8_t>(library->busBrktRight()));

    CellSeq cells;
    ConcreteLibraryCellIterator *cell_iter = library->cellIterator();
    while (cell_iter->hasNext())
      cells.push_back(reinterpret_cast<Cell*>(cell_iter->next()));
    delete cell_iter;
    writer_.putU32(static_cast<uint32_t>(cells.size()));
    for (const Cell *cell : cells)
      writeCell(cell);
  }
}

void
DbNetworkWriter::writeInstances()
{
  writer_.putU32(static_cast<uint32_t>(cells_.size()));
  for (const Cell *cell : cells_) {
    writer_.putStr(network_->name(network_->library(cell)));
    writer_.putStr(network_->name(cell));
  }

  writer_.putU32(static_cast<uint32_t>(instances_.size()));
  for (Instance *inst : instances_) {
    Instance *parent = network_->parent(inst);
    DbInstanceRec rec;
    rec.name = writer_.strings()->intern(network_->name(inst));
    rec.cell = cell_ids_[network_->cell(inst)];
    rec.parent = parent ? instance_ids_[parent] : db_network_id_null;
    visit(writer_, rec);

    // Includes the "src" attribute that report_json exposes as verilog_src.
    // AttributeMap is std::map, so key order is stable across writes.
    const AttributeMap &attrs = network_->attributeMap(inst);
    writer_.putU32(static_cast<uint32_t>(attrs.size()));
    for (const auto &[key, value] : attrs) {
      writer_.putStr(key);
      writer_.putStr(value);
    }
  }
}

void
DbNetworkWriter::writeNets()
{
  writer_.putU32(static_cast<uint32_t>(nets_.size()));
  for (Net *net : nets_) {
    Net *merged_into = network_->mergedInto(net);
    DbNetRec rec;
    rec.name = writer_.strings()->intern(network_->name(net));
    rec.instance = instance_ids_[network_->instance(net)];
    rec.merged_into = merged_into ? net_ids_[merged_into] : db_network_id_null;
    if (network_->isGround(net))
      rec.constant = stadb_constant_zero;
    else if (network_->isPower(net))
      rec.constant = stadb_constant_one;
    visit(writer_, rec);
  }
}

void
DbNetworkWriter::writePins()
{
  writer_.putU32(static_cast<uint32_t>(pins_.size()));
  for (Pin *pin : pins_) {
    Net *net = network_->net(pin);
    Term *term = network_->term(pin);
    Net *term_net = term ? network_->net(term) : nullptr;
    ConcretePort *cport = reinterpret_cast<ConcretePort*>(network_->port(pin));
    DbPinRec rec;
    rec.instance = instance_ids_[network_->instance(pin)];
    rec.pin_index = cport->pinIndex();
    rec.net = net ? net_ids_[net] : db_network_id_null;
    rec.has_term = term != nullptr;
    rec.term_net = term_net ? net_ids_[term_net] : db_network_id_null;
    visit(writer_, rec);
  }

  writer_.putU32(static_cast<uint32_t>(term_pins_.size()));
  for (DbNetworkId pin_id : term_pins_)
    writer_.putU32(pin_id);
}

void
DbNetworkWriter::write()
{
  collect();
  writeLibraries();
  writeInstances();
  writeNets();
  writePins();
}

////////////////////////////////////////////////////////////////

void
writeStaDb(std::string_view filename,
           bool compress,
           Sta *sta)
{
  Stats stats(sta->debug(), sta->report());
  debugPrint(sta->debug(), "stadb", 1, "write {}", filename);
  const SceneSeq &scenes = sta->scenes();
  if (scenes.size() != stadb_scene_count)
    throw DbUnsupported(sta::format("stadb supports a single scene but the "
                                    "session has {}", scenes.size()));

  DbFileWriter file;

  std::vector<LibertyLibrary*> libraries;
  LibertyLibraryIterator *lib_iter = sta->network()->libertyLibraryIterator();
  while (lib_iter->hasNext())
    libraries.push_back(lib_iter->next());
  delete lib_iter;

  DbWriter liberty_writer(file.strings());
  DbLibertyWriter liberty(liberty_writer, sta);
  liberty.write(libraries);
  file.addSection(DbSectionId::liberty, liberty_writer.takeBytes());

  // Only a linked network is worth storing; an unlinked one is just the
  // liberty cells that the section above already holds.
  ConcreteNetwork *network = dynamic_cast<ConcreteNetwork*>(sta->network());
  if (network && network->isLinked()) {
    DbWriter network_writer(file.strings());
    DbNetworkWriter net_section(network_writer, network);
    net_section.write();
    file.addSection(DbSectionId::network, network_writer.takeBytes());

    // Constraints name network objects, so they are only meaningful alongside
    // a network.
    DbWriter sdc_writer(file.strings());
    writeStaDbSdc(sdc_writer, sta);
    file.addSection(DbSectionId::sdc, sdc_writer.takeBytes());

    // The graph is only present once something has asked for timing. Writing
    // before that is legitimate, and just produces a smaller file that costs
    // the reader a graph build.
    if (sta->graph()) {
      DbWriter graph_writer(file.strings());
      writeStaDbGraph(graph_writer, sta);
      file.addSection(DbSectionId::graph, graph_writer.takeBytes());

      // Arrivals are what make a restore useful, but they are also the part
      // most likely to be absent: the graph exists as soon as anything
      // levelizes, long before a search has run.
      if (sta->search() && sta->search()->arrivalsValid()) {
        DbWriter search_writer(file.strings());
        writeStaDbSearch(search_writer, sta);
        file.addSection(DbSectionId::search, search_writer.takeBytes());
      }
    }
  }

  file.write(filename, compress);
  stats.report("Write sta db");
}

} // namespace sta
