// OpenSTA, Static Timing Analyzer
// Copyright (c) 2026, Silimate, Inc.
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

#include <memory>
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
#include "MinMax.hh"
#include "Network.hh"
#include "PortDirection.hh"
#include "Scene.hh"
#include "Sequential.hh"
#include "Sta.hh"
#include "Stats.hh"
#include "TableModel.hh"
#include "TimingArc.hh"
#include "TimingRole.hh"
#include "Units.hh"
#include "liberty/LibertyBuilder.hh"

namespace sta {

// Bus bits are generated from the index range instead of being listed, so the
// range is the one count in the format that costs no bytes to inflate. Two int
// fields can otherwise ask for billions of ports out of a handful of bytes.
static void
checkBusRange(int from_index,
              int to_index)
{
  int64_t from = from_index;
  int64_t to = to_index;
  int64_t width = (from < to ? to - from : from - to) + 1;
  if (width > stadb_max_bus_width)
    throw DbCorrupt(sta::format("stadb bus port has {} bits", width));
}

// Rebuilds liberty libraries by replaying the same construction calls that
// LibertyReader makes when parsing a .lib, so derived state ends up identical
// without being stored.
class DbLibertyReader
{
public:
  DbLibertyReader(DbReader &reader, Sta *sta);
  void read();

private:
  void readPools();
  LibertyLibrary *readLibrary();
  void readCell(LibertyLibrary *library);
  void readPortStructure(LibertyCell *cell);
  void readPortAttrs(LibertyPort *port);
  void readArcSet(LibertyCell *cell);
  void readInternalPower(LibertyCell *cell);
  void readLeakagePower(LibertyCell *cell);
  void readStatetable(LibertyCell *cell);
  void readModeDefs(LibertyCell *cell);
  void readOcvDerates(LibertyCell *cell);
  void readOcvDerates(LibertyLibrary *library);
  void readOcvDerate(OcvDerate *derate);
  void readCellBody(LibertyCell *cell);
  TimingModel *readModel(LibertyCell *cell);
  TableModels *readModels();
  TableModel *readTableModel();
  FuncExpr *readFuncExpr();
  void readUnit(Unit *unit);

  TableAxisPtr axis(DbPoolId id) const;
  TablePtr table(DbPoolId id) const;
  LibertyPort *port(uint32_t id) const;

  DbReader &reader_;
  Sta *sta_;
  LibertyBuilder builder_;
  LibertyLibrary *library_;
  std::vector<TableAxisPtr> axes_;
  std::vector<TablePtr> tables_;
  // Canonical port order for the cell being read, index 0 unused so that id 0
  // can mean null.
  std::vector<LibertyPort*> ports_;
};

DbLibertyReader::DbLibertyReader(DbReader &reader, Sta *sta) :
  reader_(reader),
  sta_(sta),
  builder_(sta->debug(), sta->report()),
  library_(nullptr)
{
}

TableAxisPtr
DbLibertyReader::axis(DbPoolId id) const
{
  if (id == 0)
    return nullptr;
  if (id > axes_.size())
    throw DbCorrupt("stadb liberty axis id out of range");
  return axes_[id - 1];
}

TablePtr
DbLibertyReader::table(DbPoolId id) const
{
  if (id == 0)
    return nullptr;
  if (id > tables_.size())
    throw DbCorrupt("stadb liberty table id out of range");
  return tables_[id - 1];
}

LibertyPort *
DbLibertyReader::port(uint32_t id) const
{
  if (id == 0)
    return nullptr;
  if (id >= ports_.size())
    throw DbCorrupt("stadb liberty port id out of range");
  return ports_[id];
}

void
DbLibertyReader::readPools()
{
  size_t axis_count = reader_.getCount("liberty axis");
  axes_.reserve(axis_count);
  for (size_t i = 0; i < axis_count; i++) {
    uint8_t variable = reader_.getU8();
    if (variable > static_cast<uint8_t>(TableAxisVariable::unknown))
      throw DbCorrupt("stadb liberty table axis variable out of range");
    size_t value_count = reader_.getCount("liberty axis value");
    FloatSeq values;
    values.reserve(value_count);
    for (size_t v = 0; v < value_count; v++)
      values.push_back(reader_.getF32());
    axes_.push_back(std::make_shared<TableAxis>(
                      static_cast<TableAxisVariable>(variable),
                      std::move(values)));
  }

  size_t table_count = reader_.getCount("liberty table");
  tables_.reserve(table_count);
  for (size_t i = 0; i < table_count; i++) {
    int order = reader_.getU8();
    TableAxisPtr axis1 = axis(reader_.getU32());
    TableAxisPtr axis2 = axis(reader_.getU32());
    TableAxisPtr axis3 = axis(reader_.getU32());
    switch (order) {
    case 0:
      tables_.push_back(std::make_shared<Table>(reader_.getF32()));
      break;
    case 1: {
      size_t value_count = reader_.getCount("liberty table value");
      FloatSeq values;
      values.reserve(value_count);
      for (size_t v = 0; v < value_count; v++)
        values.push_back(reader_.getF32());
      tables_.push_back(std::make_shared<Table>(std::move(values), axis1));
      break;
    }
    case 2:
    case 3: {
      size_t row_count = reader_.getCount("liberty table row");
      FloatTable values;
      values.reserve(row_count);
      for (size_t r = 0; r < row_count; r++) {
        size_t col_count = reader_.getCount("liberty table column");
        FloatSeq row;
        row.reserve(col_count);
        for (size_t c = 0; c < col_count; c++)
          row.push_back(reader_.getF32());
        values.push_back(std::move(row));
      }
      if (order == 2)
        tables_.push_back(std::make_shared<Table>(std::move(values), axis1, axis2));
      else
        tables_.push_back(std::make_shared<Table>(std::move(values), axis1, axis2,
                                                  axis3));
      break;
    }
    default:
      throw DbCorrupt("stadb liberty table order out of range");
    }
  }
}

TableModel *
DbLibertyReader::readTableModel()
{
  DbTableModelRec rec;
  visit(reader_, rec);
  TablePtr tbl = table(rec.table);
  std::string_view template_name = reader_.strings()->string(rec.tbl_template);
  if (rec.template_type >= table_template_type_count)
    throw DbCorrupt("stadb liberty table template type out of range");
  TableTemplateType type = static_cast<TableTemplateType>(rec.template_type);
  TableTemplate *tmpl = template_name.empty()
    ? nullptr : library_->findTableTemplate(template_name, type);
  const RiseFall *rf = RiseFall::find(rec.rf_index);
  if (rf == nullptr)
    throw DbCorrupt("stadb liberty table model transition out of range");
  return new TableModel(tbl, tmpl,
                        static_cast<ScaleFactorType>(rec.scale_factor_type), rf);
}

TableModels *
DbLibertyReader::readModels()
{
  uint8_t present = reader_.getU8();
  TableModels *models = new TableModels();
  if (present & stadb_models_model)
    models->setModel(readTableModel());
  TableModel *sigma_early = nullptr;
  if (present & stadb_models_sigma_early)
    sigma_early = readTableModel();
  TableModel *sigma_late = nullptr;
  if (present & stadb_models_sigma_late)
    sigma_late = readTableModel();
  // The bundle destructor deletes early and late once when they alias, so the
  // aliasing has to be restored rather than duplicated.
  if (present & stadb_models_sigma_aliased)
    sigma_late = sigma_early;
  if (sigma_early)
    models->setSigma(sigma_early, EarlyLate::early());
  if (sigma_late)
    models->setSigma(sigma_late, EarlyLate::late());
  if (present & stadb_models_std_dev)
    models->setStdDev(readTableModel());
  if (present & stadb_models_mean_shift)
    models->setMeanShift(readTableModel());
  if (present & stadb_models_skewness)
    models->setSkewness(readTableModel());
  return models;
}

TimingModel *
DbLibertyReader::readModel(LibertyCell *cell)
{
  DbModelKind kind = static_cast<DbModelKind>(reader_.getU8());
  switch (kind) {
  case DbModelKind::none:
    return nullptr;
  case DbModelKind::gate_table: {
    TableModels *delay_models = reader_.getBool() ? readModels() : nullptr;
    TableModels *slew_models = reader_.getBool() ? readModels() : nullptr;
    return new GateTableModel(cell, delay_models, slew_models);
  }
  case DbModelKind::check_table: {
    TableModels *check_models = reader_.getBool() ? readModels() : nullptr;
    return new CheckTableModel(cell, check_models);
  }
  default:
    throw DbCorrupt("stadb liberty timing model kind out of range");
  }
}

FuncExpr *
DbLibertyReader::readFuncExpr()
{
  DbFuncKind kind = static_cast<DbFuncKind>(reader_.getU8());
  switch (kind) {
  case DbFuncKind::null:
    return nullptr;
  case DbFuncKind::port: {
    LibertyPort *func_port = port(reader_.getU32());
    return func_port ? FuncExpr::makePort(func_port) : nullptr;
  }
  case DbFuncKind::not_:
    return FuncExpr::makeNot(readFuncExpr());
  case DbFuncKind::or_: {
    FuncExpr *left = readFuncExpr();
    return FuncExpr::makeOr(left, readFuncExpr());
  }
  case DbFuncKind::and_: {
    FuncExpr *left = readFuncExpr();
    return FuncExpr::makeAnd(left, readFuncExpr());
  }
  case DbFuncKind::xor_: {
    FuncExpr *left = readFuncExpr();
    return FuncExpr::makeXor(left, readFuncExpr());
  }
  case DbFuncKind::one:
    return FuncExpr::makeOne();
  case DbFuncKind::zero:
    return FuncExpr::makeZero();
  }
  throw DbCorrupt("stadb liberty function expression kind out of range");
}

void
DbLibertyReader::readUnit(Unit *unit)
{
  unit->setScale(reader_.getF32());
  std::string suffix(reader_.getStr());
  unit->setSuffix(suffix.c_str());
  unit->setDigits(reader_.getI32());
}

////////////////////////////////////////////////////////////////

void
DbLibertyReader::readPortStructure(LibertyCell *cell)
{
  std::string name(reader_.getStr());
  DbPortKind kind = static_cast<DbPortKind>(reader_.getU8());
  switch (kind) {
  case DbPortKind::scalar:
    builder_.makePort(cell, name);
    break;
  case DbPortKind::bus: {
    int from_index = reader_.getI32();
    int to_index = reader_.getI32();
    std::string bus_dcl_name(reader_.getStr());
    BusDcl *bus_dcl = bus_dcl_name.empty()
      ? nullptr : library_->findBusDcl(bus_dcl_name);
    checkBusRange(from_index, to_index);
    builder_.makeBusPort(cell, name, from_index, to_index, bus_dcl);
    break;
  }
  case DbPortKind::bundle: {
    uint32_t member_count = reader_.getU32();
    ConcretePortSeq *members = new ConcretePortSeq;
    for (uint32_t i = 0; i < member_count; i++) {
      std::string member_name(reader_.getStr());
      LibertyPort *member = cell->findLibertyPort(member_name);
      if (member == nullptr)
        throw DbCorrupt("stadb liberty bundle member not found");
      members->push_back(member);
    }
    builder_.makeBundlePort(cell, name, members);
    break;
  }
  default:
    throw DbCorrupt("stadb liberty port kind out of range");
  }
}

void
DbLibertyReader::readPortAttrs(LibertyPort *lib_port)
{
  DbPortRec rec;
  visit(reader_, rec);

  std::string_view dir_name = reader_.strings()->string(rec.direction);
  if (!dir_name.empty()) {
    std::string dir(dir_name);
    PortDirection *direction = PortDirection::find(dir.c_str());
    if (direction)
      lib_port->setDirection(direction);
  }
  lib_port->setPwrGndType(static_cast<PwrGndType>(rec.pwr_gnd_type));
  std::string_view voltage_name = reader_.strings()->string(rec.voltage_name);
  if (!voltage_name.empty())
    lib_port->setVoltageName(voltage_name);
  lib_port->setScanSignalType(static_cast<ScanSignalType>(rec.scan_signal_type));
  if (rec.fanout_load_exists)
    lib_port->setFanoutLoad(rec.fanout_load);
  lib_port->setMinPeriod(rec.min_period);
  lib_port->setIsClock(rec.is_clk);
  lib_port->setIsRegClk(rec.is_reg_clk);
  lib_port->setIsCheckClk(rec.is_check_clk);
  lib_port->setIsRegOutput(rec.is_reg_output);
  lib_port->setIsLatchData(rec.is_latch_data);
  lib_port->setIsLatchOutput(rec.is_latch_output);
  lib_port->setIsClockGateClock(rec.is_clk_gate_clk);
  lib_port->setIsClockGateEnable(rec.is_clk_gate_enable);
  lib_port->setIsClockGateOut(rec.is_clk_gate_out);
  lib_port->setIsPllFeedback(rec.is_pll_feedback);
  lib_port->setIsSwitch(rec.is_switch);
  lib_port->setIsPad(rec.is_pad);
  lib_port->setIsolationCellData(rec.isolation_cell_data);
  lib_port->setIsolationCellEnable(rec.isolation_cell_enable);
  lib_port->setLevelShifterData(rec.level_shifter_data);
  if (rec.pulse_clk_trigger != 0 && rec.pulse_clk_sense != 0) {
    const RiseFall *trigger = RiseFall::find(rec.pulse_clk_trigger - 1);
    const RiseFall *sense = RiseFall::find(rec.pulse_clk_sense - 1);
    if (trigger && sense)
      lib_port->setPulseClk(trigger, sense);
  }

  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *mm : MinMax::range()) {
      if (reader_.getBool())
        lib_port->setCapacitance(rf, mm, reader_.getF32());
    }
  }
  for (const MinMax *mm : MinMax::range()) {
    if (reader_.getBool())
      lib_port->setSlewLimit(reader_.getF32(), mm);
    if (reader_.getBool())
      lib_port->setCapacitanceLimit(reader_.getF32(), mm);
    if (reader_.getBool())
      lib_port->setFanoutLimit(reader_.getF32(), mm);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    if (reader_.getBool())
      lib_port->setMinPulseWidth(rf, reader_.getF32());
  }
  FuncExpr *function = readFuncExpr();
  if (function)
    lib_port->setFunction(function);
  FuncExpr *tristate_enable = readFuncExpr();
  if (tristate_enable)
    lib_port->setTristateEnable(tristate_enable);
  LibertyPort *ground_port = port(reader_.getU32());
  if (ground_port)
    lib_port->setRelatedGroundPort(ground_port);
  LibertyPort *power_port = port(reader_.getU32());
  if (power_port)
    lib_port->setRelatedPowerPort(power_port);
}

void
DbLibertyReader::readArcSet(LibertyCell *cell)
{
  DbArcSetRec rec;
  visit(reader_, rec);

  LibertyPort *from = port(rec.from_port);
  LibertyPort *to = port(rec.to_port);
  LibertyPort *related_out = port(rec.related_out_port);
  std::string role_name(reader_.strings()->string(rec.role));
  const TimingRole *role = TimingRole::find(role_name.c_str());
  if (role == nullptr)
    throw DbCorrupt(sta::format("stadb liberty timing role {} is unknown",
                                role_name));

  TimingArcAttrsPtr attrs = std::make_shared<TimingArcAttrs>();
  attrs->setTimingType(findTimingType(reader_.strings()->string(rec.timing_type)));
  if (rec.timing_sense >= timing_sense_count)
    throw DbCorrupt("stadb liberty timing sense out of range");
  attrs->setTimingSense(static_cast<TimingSense>(rec.timing_sense));
  attrs->setSdfCond(reader_.strings()->string(rec.sdf_cond));
  attrs->setSdfCondStart(reader_.strings()->string(rec.sdf_cond_start));
  attrs->setSdfCondEnd(reader_.strings()->string(rec.sdf_cond_end));
  attrs->setModeName(reader_.strings()->string(rec.mode_name));
  attrs->setModeValue(reader_.strings()->string(rec.mode_value));
  attrs->setOcvArcDepth(rec.ocv_arc_depth);
  FuncExpr *cond = readFuncExpr();
  if (cond)
    attrs->setCond(cond);

  TimingArcSet *arc_set = cell->makeTimingArcSet(from, to, related_out, role,
                                                 attrs);
  uint32_t arc_count = reader_.getU32();
  for (uint32_t i = 0; i < arc_count; i++) {
    std::string from_name(reader_.getStr());
    std::string to_name(reader_.getStr());
    const Transition *from_rf = Transition::find(from_name);
    const Transition *to_rf = Transition::find(to_name);
    if (from_rf == nullptr || to_rf == nullptr)
      throw DbCorrupt("stadb liberty timing arc transition is unknown");
    TimingModel *model = readModel(cell);
    // The constructor registers the arc with its set.
    new TimingArc(arc_set, from_rf, to_rf, model);
  }
}

void
DbLibertyReader::readInternalPower(LibertyCell *cell)
{
  LibertyPort *lib_port = port(reader_.getU32());
  LibertyPort *related_port = port(reader_.getU32());
  LibertyPort *related_pg_pin = port(reader_.getU32());
  std::shared_ptr<FuncExpr> when(readFuncExpr());
  uint8_t present = reader_.getU8();
  InternalPowerModels models{};
  std::shared_ptr<TableModel> rise_model;
  if (present & stadb_internal_power_rise)
    rise_model.reset(readTableModel());
  std::shared_ptr<TableModel> fall_model;
  if (present & stadb_internal_power_aliased)
    fall_model = rise_model;
  else if (present & stadb_internal_power_fall)
    fall_model.reset(readTableModel());
  if (rise_model)
    models[RiseFall::riseIndex()] = InternalPowerModel(rise_model);
  if (fall_model)
    models[RiseFall::fallIndex()] = InternalPowerModel(fall_model);
  if (lib_port == nullptr)
    throw DbCorrupt("stadb liberty internal power port is missing");
  cell->makeInternalPower(lib_port, related_port, related_pg_pin, when, models);
}

void
DbLibertyReader::readLeakagePower(LibertyCell *cell)
{
  LibertyPort *related_pg_port = port(reader_.getU32());
  FuncExpr *when = readFuncExpr();
  float power = reader_.getF32();
  cell->makeLeakagePower(related_pg_port, when, power);
}

void
DbLibertyReader::readStatetable(LibertyCell *cell)
{
  uint32_t input_count = reader_.getU32();
  LibertyPortSeq input_ports;
  input_ports.reserve(input_count);
  for (uint32_t i = 0; i < input_count; i++) {
    LibertyPort *lib_port = port(reader_.getU32());
    if (lib_port == nullptr)
      throw DbCorrupt("stadb liberty statetable input port is missing");
    input_ports.push_back(lib_port);
  }
  uint32_t internal_count = reader_.getU32();
  LibertyPortSeq internal_ports;
  internal_ports.reserve(internal_count);
  for (uint32_t i = 0; i < internal_count; i++) {
    LibertyPort *lib_port = port(reader_.getU32());
    if (lib_port == nullptr)
      throw DbCorrupt("stadb liberty statetable internal port is missing");
    internal_ports.push_back(lib_port);
  }
  uint32_t row_count = reader_.getU32();
  StatetableRows table;
  table.reserve(row_count);
  for (uint32_t i = 0; i < row_count; i++) {
    uint32_t input_value_count = reader_.getU32();
    StateInputValues input_values;
    input_values.reserve(input_value_count);
    for (uint32_t j = 0; j < input_value_count; j++)
      input_values.push_back(static_cast<StateInputValue>(reader_.getU8()));
    uint32_t current_value_count = reader_.getU32();
    StateInternalValues current_values;
    current_values.reserve(current_value_count);
    for (uint32_t j = 0; j < current_value_count; j++)
      current_values.push_back(static_cast<StateInternalValue>(reader_.getU8()));
    uint32_t next_value_count = reader_.getU32();
    StateInternalValues next_values;
    next_values.reserve(next_value_count);
    for (uint32_t j = 0; j < next_value_count; j++)
      next_values.push_back(static_cast<StateInternalValue>(reader_.getU8()));
    table.emplace_back(input_values, current_values, next_values);
  }
  cell->makeStatetable(input_ports, internal_ports, table);
}

void
DbLibertyReader::readModeDefs(LibertyCell *cell)
{
  uint32_t mode_count = reader_.getU32();
  for (uint32_t i = 0; i < mode_count; i++) {
    std::string name(reader_.getStr());
    ModeDef *mode_def = cell->makeModeDef(name);
    uint32_t value_count = reader_.getU32();
    for (uint32_t j = 0; j < value_count; j++) {
      std::string value_name(reader_.getStr());
      ModeValueDef *value_def = mode_def->defineValue(value_name);
      std::string sdf_cond(reader_.getStr());
      if (!sdf_cond.empty())
        value_def->setSdfCond(sdf_cond);
      FuncExpr *cond = readFuncExpr();
      if (cond)
        value_def->setCond(cond);
    }
  }
}

void
DbLibertyReader::readOcvDerate(OcvDerate *derate)
{
  for (const RiseFall *rf : RiseFall::range()) {
    for (const EarlyLate *early_late : EarlyLate::range()) {
      for (int path = 0; path < path_type_count; path++) {
        TablePtr tbl = table(reader_.getU32());
        if (tbl)
          derate->setDerateTable(rf, early_late,
                                 static_cast<PathType>(path), tbl);
      }
    }
  }
}

void
DbLibertyReader::readOcvDerates(LibertyLibrary *library)
{
  uint32_t count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    std::string name(reader_.getStr());
    OcvDerate *derate = library->makeOcvDerate(name);
    readOcvDerate(derate);
  }
}

void
DbLibertyReader::readOcvDerates(LibertyCell *cell)
{
  uint32_t count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    std::string name(reader_.getStr());
    OcvDerate *derate = cell->makeOcvDerate(name);
    readOcvDerate(derate);
  }
}

void
DbLibertyReader::readCell(LibertyLibrary *library)
{
  DbCellRec rec;
  visit(reader_, rec);

  std::string name(reader_.strings()->string(rec.name));
  std::string filename(reader_.strings()->string(rec.filename));
  LibertyCell *cell = builder_.makeCell(library, name, filename);
  cell->setArea(rec.area);
  cell->setDontUse(rec.dont_use);
  cell->setIsMacro(rec.is_macro);
  cell->setIsMemory(rec.is_memory);
  cell->setIsPad(rec.is_pad);
  cell->setIsClockCell(rec.is_clock_cell);
  cell->setIsLevelShifter(rec.is_level_shifter);
  cell->setLevelShifterType(static_cast<LevelShifterType>(rec.level_shifter_type));
  cell->setIsIsolationCell(rec.is_isolation_cell);
  cell->setAlwaysOn(rec.always_on);
  cell->setSwitchCellType(static_cast<SwitchCellType>(rec.switch_cell_type));
  cell->setInterfaceTiming(rec.interface_timing);
  cell->setClockGateType(static_cast<ClockGateType>(rec.clock_gate_type));
  cell->setHasInferedRegTimingArcs(rec.has_infered_reg_timing_arcs);
  cell->setOcvArcDepth(rec.ocv_arc_depth);
  cell->setFootprint(reader_.strings()->string(rec.footprint));
  cell->setUserFunctionClass(reader_.strings()->string(rec.user_function_class));
  if (rec.leakage_power_exists)
    cell->setLeakagePower(rec.leakage_power);

  readCellBody(cell);
}

void
DbLibertyReader::readCellBody(LibertyCell *cell)
{
  uint32_t container_count = reader_.getU32();
  for (uint32_t i = 0; i < container_count; i++)
    readPortStructure(cell);

  // Rebuild the canonical port order the writer used for port ids.
  ports_.assign(1, nullptr);
  LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    LibertyPort *lib_port = port_iter.next();
    ports_.push_back(lib_port);
    if (lib_port->hasMembers()) {
      LibertyPortMemberIterator member_iter(lib_port);
      while (member_iter.hasNext())
        ports_.push_back(member_iter.next());
    }
  }
  uint32_t port_count = reader_.getU32();
  if (port_count + 1 != ports_.size())
    throw DbCorrupt("stadb liberty cell port count does not match structure");
  for (size_t i = 1; i < ports_.size(); i++)
    readPortAttrs(ports_[i]);
  // LibertyReader sets these from clock_gate_* port attrs; replay them here.
  for (size_t i = 1; i < ports_.size(); i++) {
    LibertyPort *lib_port = ports_[i];
    if (lib_port->isClockGateClock())
      cell->setHasClkGateClkPin();
    if (lib_port->isClockGateEnable())
      cell->setHasClkGateEnablePin();
  }

  uint32_t sequential_count = reader_.getU32();
  for (uint32_t i = 0; i < sequential_count; i++) {
    bool is_register = reader_.getBool();
    FuncExpr *clk = readFuncExpr();
    FuncExpr *data = readFuncExpr();
    FuncExpr *clear = readFuncExpr();
    FuncExpr *preset = readFuncExpr();
    LogicValue clr_preset_out = static_cast<LogicValue>(reader_.getU8());
    LogicValue clr_preset_out_inv = static_cast<LogicValue>(reader_.getU8());
    LibertyPort *output = port(reader_.getU32());
    LibertyPort *output_inv = port(reader_.getU32());
    // Size 1 because the stored sequentials are already bit split.
    cell->makeSequential(1, is_register, clk, data, clear, preset,
                         clr_preset_out, clr_preset_out_inv, output, output_inv);
  }

  if (reader_.getBool()) {
    DbCellRec rec;
    visit(reader_, rec);
    std::string name(reader_.strings()->string(rec.name));
    std::string filename(reader_.strings()->string(rec.filename));
    TestCell *test_cell = new TestCell(cell->libertyLibrary(), name, filename);
    test_cell->setArea(rec.area);
    test_cell->setDontUse(rec.dont_use);
    test_cell->setIsMacro(rec.is_macro);
    test_cell->setIsMemory(rec.is_memory);
    test_cell->setIsPad(rec.is_pad);
    test_cell->setIsClockCell(rec.is_clock_cell);
    test_cell->setIsLevelShifter(rec.is_level_shifter);
    test_cell->setLevelShifterType(
        static_cast<LevelShifterType>(rec.level_shifter_type));
    test_cell->setIsIsolationCell(rec.is_isolation_cell);
    test_cell->setAlwaysOn(rec.always_on);
    test_cell->setSwitchCellType(
        static_cast<SwitchCellType>(rec.switch_cell_type));
    test_cell->setInterfaceTiming(rec.interface_timing);
    test_cell->setClockGateType(
        static_cast<ClockGateType>(rec.clock_gate_type));
    test_cell->setHasInferedRegTimingArcs(rec.has_infered_reg_timing_arcs);
    test_cell->setOcvArcDepth(rec.ocv_arc_depth);
    test_cell->setFootprint(reader_.strings()->string(rec.footprint));
    test_cell->setUserFunctionClass(
        reader_.strings()->string(rec.user_function_class));
    if (rec.leakage_power_exists)
      test_cell->setLeakagePower(rec.leakage_power);
    cell->setTestCell(test_cell);
    std::vector<LibertyPort*> saved_ports = ports_;
    readCellBody(test_cell);
    ports_ = std::move(saved_ports);
  }

  uint32_t arc_set_count = reader_.getU32();
  for (uint32_t i = 0; i < arc_set_count; i++)
    readArcSet(cell);

  uint32_t internal_power_count = reader_.getU32();
  for (uint32_t i = 0; i < internal_power_count; i++)
    readInternalPower(cell);

  uint32_t leakage_power_count = reader_.getU32();
  for (uint32_t i = 0; i < leakage_power_count; i++)
    readLeakagePower(cell);

  if (reader_.getBool())
    readStatetable(cell);

  readModeDefs(cell);
  readOcvDerates(cell);
  std::string ocv_group_name(reader_.getStr());
  if (!ocv_group_name.empty()) {
    OcvDerate *derate = cell->findOcvDerate(ocv_group_name);
    if (derate == nullptr)
      derate = cell->libertyLibrary()->findOcvDerate(ocv_group_name);
    if (derate == nullptr)
      throw DbCorrupt(sta::format("stadb liberty ocv derate {} not found",
                                  ocv_group_name));
    cell->setOcvDerate(derate);
  }

  // Rebuilds the port/arc maps, latch enables and default cond arcs rather than
  // storing them.
  cell->finish(false, sta_->report(), sta_->debug());
}

LibertyLibrary *
DbLibertyReader::readLibrary()
{
  std::string name(reader_.getStr());
  std::string filename(reader_.getStr());
  LibertyLibrary *library = sta_->network()->makeLibertyLibrary(name, filename);
  library_ = library;
  library->setDelayModelType(static_cast<DelayModelType>(reader_.getU8()));

  Units *units = library->units();
  readUnit(units->timeUnit());
  readUnit(units->capacitanceUnit());
  readUnit(units->voltageUnit());
  readUnit(units->resistanceUnit());
  readUnit(units->currentUnit());
  readUnit(units->powerUnit());
  readUnit(units->distanceUnit());

  library->setNominalProcess(reader_.getF32());
  library->setNominalVoltage(reader_.getF32());
  library->setNominalTemperature(reader_.getF32());
  library->setDefaultInputPinCap(reader_.getF32());
  library->setDefaultOutputPinCap(reader_.getF32());
  library->setDefaultBidirectPinCap(reader_.getF32());
  {
    bool exists = reader_.getBool();
    float value = reader_.getF32();
    if (exists)
      library->setDefaultFanoutLoad(value);
    exists = reader_.getBool();
    value = reader_.getF32();
    if (exists)
      library->setDefaultMaxSlew(value);
    exists = reader_.getBool();
    value = reader_.getF32();
    if (exists)
      library->setDefaultMaxCapacitance(value);
    exists = reader_.getBool();
    value = reader_.getF32();
    if (exists)
      library->setDefaultMaxFanout(value);
  }
  library->setSlewDerateFromLibrary(reader_.getF32());
  library->setOcvArcDepth(reader_.getF32());
  for (const RiseFall *rf : RiseFall::range()) {
    library->setInputThreshold(rf, reader_.getF32());
    library->setOutputThreshold(rf, reader_.getF32());
    library->setSlewLowerThreshold(rf, reader_.getF32());
    library->setSlewUpperThreshold(rf, reader_.getF32());
  }

  if (reader_.getBool()) {
    std::string op_cond_name(reader_.getStr());
    OperatingConditions *op_cond = library->makeOperatingConditions(op_cond_name);
    op_cond->setProcess(reader_.getF32());
    op_cond->setVoltage(reader_.getF32());
    op_cond->setTemperature(reader_.getF32());
    op_cond->setWireloadTree(static_cast<WireloadTree>(reader_.getU8()));
    library->setDefaultOperatingConditions(op_cond);
  }

  if (reader_.getBool()) {
    std::string scales_name(reader_.getStr());
    uint8_t type_count = reader_.getU8();
    uint8_t pvt_count = reader_.getU8();
    if (type_count != scale_factor_type_count || pvt_count != scale_factor_pvt_count)
      throw DbCorrupt(sta::format("stadb scale factor dimensions {}x{} do not "
                                  "match this build's {}x{}", type_count, pvt_count,
                                  scale_factor_type_count, scale_factor_pvt_count));
    ScaleFactors *scales = library->makeScaleFactors(scales_name);
    for (int type = 0; type < type_count; type++) {
      for (int pvt = 0; pvt < pvt_count; pvt++) {
        for (const RiseFall *rf : RiseFall::range())
          scales->setScale(static_cast<ScaleFactorType>(type),
                           static_cast<ScaleFactorPvt>(pvt), rf, reader_.getF32());
      }
    }
    library->setScaleFactors(scales);
  }

  for (int i = 0; i < table_template_type_count; i++) {
    TableTemplateType type = static_cast<TableTemplateType>(i);
    uint32_t template_count = reader_.getU32();
    for (uint32_t t = 0; t < template_count; t++) {
      std::string template_name(reader_.getStr());
      TableAxisPtr axis1 = axis(reader_.getU32());
      TableAxisPtr axis2 = axis(reader_.getU32());
      TableAxisPtr axis3 = axis(reader_.getU32());
      // The constructor pre-creates "scalar", so this returns the existing
      // template for that name rather than duplicating it.
      TableTemplate *tmpl = library->makeTableTemplate(template_name, type);
      if (axis1)
        tmpl->setAxis1(axis1);
      if (axis2)
        tmpl->setAxis2(axis2);
      if (axis3)
        tmpl->setAxis3(axis3);
    }
  }

  readOcvDerates(library);
  std::string default_ocv(reader_.getStr());
  if (!default_ocv.empty()) {
    OcvDerate *derate = library->findOcvDerate(default_ocv);
    if (derate == nullptr)
      throw DbCorrupt(sta::format("stadb liberty default ocv derate {} not found",
                                  default_ocv));
    library->setDefaultOcvDerate(derate);
  }

  uint32_t cell_count = reader_.getU32();
  for (uint32_t i = 0; i < cell_count; i++)
    readCell(library);
  return library;
}

void
DbLibertyReader::read()
{
  readPools();
  uint32_t library_count = reader_.getU32();
  Scene *scene = sta_->scenes()[0];
  for (uint32_t i = 0; i < library_count; i++) {
    LibertyLibrary *library = readLibrary();
    // Same registration Sta::readLibertyAfter performs after parsing a .lib.
    for (const MinMax *mm : MinMax::range())
      scene->addLiberty(library, mm);
    LibertyLibrary::makeSceneMap(library, scene, MinMaxAll::all(),
                                 sta_->network(), sta_->report());
    if (sta_->network()->defaultLibertyLibrary() == nullptr) {
      sta_->network()->setDefaultLibertyLibrary(library);
      // Report scaling comes from the global units, which Sta::readLiberty
      // takes from the first library. Without this every delay reports as 0
      // because the values are correct seconds displayed on a 1s scale.
      sta_->copyUnits(library->units());
    }
  }
}

////////////////////////////////////////////////////////////////

// Rebuilds the linked network by replaying the calls a reader plus link_design
// would have made, in the recorded creation order.
class DbNetworkReader
{
public:
  DbNetworkReader(DbReader &reader, ConcreteNetwork *network);
  void read();

private:
  void readLibraries();
  void readCell(Library *library);
  void readPort(Cell *cell);
  void readCellRefs();
  void readInstances();
  void readNets();
  void readPins();

  Cell *cell(uint32_t id) const;
  Instance *instance(DbNetworkId id) const;
  Net *net(DbNetworkId id) const;
  const std::vector<ConcretePort*> &bitPorts(ConcreteCell *cell);

  DbReader &reader_;
  ConcreteNetwork *network_;
  std::vector<Cell*> cells_;
  std::vector<Instance*> instances_;
  std::vector<Net*> nets_;
  Instance *top_instance_{nullptr};
  // Bit ports of a cell indexed by pin index, cached because every pin of
  // every instance of the cell resolves its port through it.
  std::unordered_map<ConcreteCell*, std::vector<ConcretePort*>> bit_ports_;
};

DbNetworkReader::DbNetworkReader(DbReader &reader, ConcreteNetwork *network) :
  reader_(reader),
  network_(network)
{
}

Cell *
DbNetworkReader::cell(uint32_t id) const
{
  if (id >= cells_.size())
    throw DbCorrupt("stadb network cell id out of range");
  return cells_[id];
}

Instance *
DbNetworkReader::instance(DbNetworkId id) const
{
  if (id >= instances_.size())
    throw DbCorrupt("stadb network instance id out of range");
  return instances_[id];
}

Net *
DbNetworkReader::net(DbNetworkId id) const
{
  if (id == db_network_id_null)
    return nullptr;
  if (id >= nets_.size())
    throw DbCorrupt("stadb network net id out of range");
  return nets_[id];
}

const std::vector<ConcretePort*> &
DbNetworkReader::bitPorts(ConcreteCell *cell)
{
  auto itr = bit_ports_.find(cell);
  if (itr != bit_ports_.end())
    return itr->second;
  std::vector<ConcretePort*> &ports = bit_ports_[cell];
  ports.resize(cell->portBitCount(), nullptr);
  ConcreteCellPortBitIterator bit_iter(cell);
  while (bit_iter.hasNext()) {
    ConcretePort *bit = bit_iter.next();
    int index = bit->pinIndex();
    if (index >= 0 && index < static_cast<int>(ports.size()))
      ports[index] = bit;
  }
  return ports;
}

void
DbNetworkReader::readPort(Cell *cell)
{
  std::string name(reader_.getStr());
  uint8_t kind = reader_.getU8();
  // Not getCstring: it hands back null for the empty string, and the lookup
  // below reads it as a C string.
  std::string dir_name(reader_.getStr());
  PortDirection *dir = dbCheck(PortDirection::find(dir_name.c_str()),
                               "network port direction");

  Port *port = nullptr;
  switch (static_cast<DbPortKind>(kind)) {
  case DbPortKind::scalar:
    port = network_->makePort(cell, name);
    break;
  case DbPortKind::bus: {
    int from_index = reader_.getI32();
    int to_index = reader_.getI32();
    checkBusRange(from_index, to_index);
    port = network_->makeBusPort(cell, name, from_index, to_index);
    break;
  }
  case DbPortKind::bundle: {
    uint32_t member_count = reader_.getU32();
    PortSeq *members = new PortSeq;
    for (uint32_t i = 0; i < member_count; i++) {
      Port *member = network_->findPort(cell, reader_.getStr());
      if (member == nullptr)
        throw DbCorrupt("stadb network bundle member not found");
      members->push_back(member);
    }
    port = network_->makeBundlePort(cell, name, members);
    break;
  }
  default:
    throw DbCorrupt("stadb network port kind out of range");
  }
  // Propagates to bus bits and bundle members, matching the writer's guard.
  network_->setDirection(port, dir);
}

void
DbNetworkReader::readCell(Library *library)
{
  std::string name(reader_.getStr());
  std::string filename(reader_.getStr());
  bool is_leaf = reader_.getBool();
  Cell *cell = network_->makeCell(library, name, is_leaf, filename);

  uint32_t attr_count = reader_.getU32();
  for (uint32_t i = 0; i < attr_count; i++) {
    std::string key(reader_.getStr());
    network_->setAttribute(cell, key, reader_.getStr());
  }

  uint32_t port_count = reader_.getU32();
  for (uint32_t i = 0; i < port_count; i++)
    readPort(cell);
}

void
DbNetworkReader::readLibraries()
{
  uint32_t library_count = reader_.getU32();
  for (uint32_t i = 0; i < library_count; i++) {
    std::string name(reader_.getStr());
    std::string filename(reader_.getStr());
    char brkt_left = static_cast<char>(reader_.getU8());
    char brkt_right = static_cast<char>(reader_.getU8());
    Library *library = network_->makeLibrary(name, filename);
    reinterpret_cast<ConcreteLibrary*>(library)->setBusBrkts(brkt_left,
                                                             brkt_right);
    uint32_t cell_count = reader_.getU32();
    for (uint32_t j = 0; j < cell_count; j++)
      readCell(library);
  }
}

void
DbNetworkReader::readCellRefs()
{
  size_t cell_count = reader_.getCount("network cell ref");
  cells_.reserve(cell_count);
  for (size_t i = 0; i < cell_count; i++) {
    std::string lib_name(reader_.getStr());
    std::string cell_name(reader_.getStr());
    Library *library = network_->findLibrary(lib_name);
    if (library == nullptr)
      throw DbCorrupt(sta::format("stadb network library {} not found",
                                  lib_name));
    Cell *cell = network_->findCell(library, cell_name);
    if (cell == nullptr)
      throw DbCorrupt(sta::format("stadb network cell {}/{} not found",
                                  lib_name, cell_name));
    cells_.push_back(cell);
  }
}

void
DbNetworkReader::readInstances()
{
  readCellRefs();
  size_t inst_count = reader_.getCount("network instance");
  instances_.reserve(inst_count);
  for (size_t i = 0; i < inst_count; i++) {
    DbInstanceRec rec;
    visit(reader_, rec);
    // Parents always precede their children because ids are handed out in
    // creation order and a parent must exist to make a child.
    Instance *parent = rec.parent == db_network_id_null
      ? nullptr
      : instance(rec.parent);
    std::string name(reader_.strings()->string(rec.name));
    Instance *inst = network_->makeInstance(cell(rec.cell), name, parent);
    instances_.push_back(inst);
    if (parent == nullptr) {
      if (top_instance_)
        throw DbCorrupt("stadb network has more than one root instance");
      top_instance_ = inst;
    }

    uint32_t attr_count = reader_.getU32();
    for (uint32_t j = 0; j < attr_count; j++) {
      std::string key(reader_.getStr());
      network_->setAttribute(inst, key, reader_.getStr());
    }
  }
}

void
DbNetworkReader::readNets()
{
  size_t net_count = reader_.getCount("network net");
  std::vector<DbNetRec> recs(net_count);
  nets_.reserve(net_count);
  for (size_t i = 0; i < net_count; i++) {
    DbNetRec &rec = recs[i];
    visit(reader_, rec);
    std::string name(reader_.strings()->string(rec.name));
    Net *net = network_->makeNet(name, instance(rec.instance));
    nets_.push_back(net);
    if (rec.constant == stadb_constant_zero)
      network_->addConstantNet(net, LogicValue::zero);
    else if (rec.constant == stadb_constant_one)
      network_->addConstantNet(net, LogicValue::one);
  }
  // Merges are replayed after every net exists so that a survivor later in the
  // array resolves. They move no pins here, since the pins are created against
  // the surviving net directly, and only restore the name alias.
  for (uint32_t i = 0; i < net_count; i++) {
    if (recs[i].merged_into != db_network_id_null)
      network_->mergeInto(nets_[i], net(recs[i].merged_into));
  }
}

void
DbNetworkReader::readPins()
{
  size_t pin_count = reader_.getCount("network pin");
  std::vector<Pin*> pins(pin_count);
  std::vector<DbNetworkId> term_nets(pin_count, db_network_id_null);
  std::vector<bool> has_term(pin_count, false);
  for (size_t i = 0; i < pin_count; i++) {
    DbPinRec rec;
    visit(reader_, rec);
    Instance *inst = instance(rec.instance);
    ConcreteCell *ccell = reinterpret_cast<ConcreteCell*>(network_->cell(inst));
    const std::vector<ConcretePort*> &ports = bitPorts(ccell);
    if (rec.pin_index < 0 || rec.pin_index >= static_cast<int>(ports.size())
        || ports[rec.pin_index] == nullptr)
      throw DbCorrupt("stadb network pin port index out of range");
    Port *port = reinterpret_cast<Port*>(ports[rec.pin_index]);
    pins[i] = network_->makePin(inst, port, net(rec.net));
    if (rec.has_term) {
      has_term[i] = true;
      term_nets[i] = rec.term_net;
    }
  }

  // Terms come last and in their own order, because a net holds its terms in
  // creation order and upstream does not create them alongside their pin.
  size_t term_count = reader_.getCount("network term");
  for (size_t i = 0; i < term_count; i++) {
    uint32_t pin_id = reader_.getU32();
    if (pin_id >= pin_count || !has_term[pin_id])
      throw DbCorrupt("stadb network term pin index out of range");
    network_->makeTerm(pins[pin_id], net(term_nets[pin_id]));
  }
}

void
DbNetworkReader::read()
{
  readLibraries();
  readInstances();
  readNets();
  readPins();
  if (top_instance_ == nullptr)
    throw DbCorrupt("stadb network has no root instance");
  // The top instance is what makes the network linked, so it goes last: an
  // exception before this point leaves an unlinked session rather than a
  // half-built one that looks ready.
  network_->setTopInstance(top_instance_);
}

////////////////////////////////////////////////////////////////

// Drop everything readStaDb is about to rebuild. Sta::clear covers graph,
// search, SDC and parasitics, but leaves liberty on the scenes and the
// network libraries in place -- both of which the liberty/network sections
// recreate from scratch.
static void
clearSession(Sta *sta)
{
  sta->clear();
  sta->clearSceneLiberty();
  sta->network()->clear();
}

static void
restoreSession(DbFileReader &file,
               Sta *sta)
{
  DbReader liberty_reader = file.sectionReader(DbSectionId::liberty);
  DbLibertyReader liberty(liberty_reader, sta);
  liberty.read();
  liberty_reader.checkFullyConsumed("liberty");

  if (file.hasSection(DbSectionId::network)) {
    ConcreteNetwork *network = dynamic_cast<ConcreteNetwork*>(sta->network());
    if (network == nullptr)
      throw DbUnsupported("stadb network restore needs a ConcreteNetwork");
    DbReader network_reader = file.sectionReader(DbSectionId::network);
    DbNetworkReader net_section(network_reader, network);
    net_section.read();
    network_reader.checkFullyConsumed("network");
  }

  // Before the constraints, even though a cold run constrains first. Creating a
  // generated clock calls updateGeneratedClks, which levelizes, which builds
  // the graph from the network -- the exact work this section exists to avoid.
  // Restoring the graph first makes that call find a levelized graph and
  // return immediately.
  if (file.hasSection(DbSectionId::graph)) {
    DbReader graph_reader = file.sectionReader(DbSectionId::graph);
    readStaDbGraph(graph_reader, sta);
    graph_reader.checkFullyConsumed("graph");
  }

  if (file.hasSection(DbSectionId::sdc)) {
    DbReader sdc_reader = file.sectionReader(DbSectionId::sdc);
    readStaDbSdc(sdc_reader, sta);
    sdc_reader.checkFullyConsumed("sdc");
  }

  // Last, because tags name the exceptions and clocks the sdc section creates.
  if (file.hasSection(DbSectionId::search)) {
    DbReader search_reader = file.sectionReader(DbSectionId::search);
    readStaDbSearch(search_reader, sta);
    search_reader.checkFullyConsumed("search");
  }
}

void
readStaDb(std::string_view filename,
          Sta *sta)
{
  Stats stats(sta->debug(), sta->report());
  debugPrint(sta->debug(), "stadb", 1, "read {}", filename);
  DbFileReader file;
  file.read(filename);

  clearSession(sta);
  try {
    restoreSession(file, sta);
  }
  catch (Exception &) {
    // Restore is in-place. Drop whatever was rebuilt so a section-level throw
    // leaves an empty session rather than a half-built one.
    clearSession(sta);
    throw;
  }
  stats.report("Read sta db");
}

} // namespace sta
