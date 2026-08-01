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

#include "LibDb.hh"

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ConcreteLibrary.hh"
#include "FuncExpr.hh"
#include "GeneratedClock.hh"
#include "Liberty.hh"
#include "PortDirection.hh"
#include "Report.hh"
#include "Sequential.hh"
#include "TableModel.hh"
#include "TimingArc.hh"
#include "TimingModel.hh"
#include "TimingRole.hh"
#include "Transition.hh"
#include "Units.hh"

namespace sta {

namespace {

constexpr uint32_t kNullId = 0xFFFFFFFFu;

enum class ModelKind : uint8_t { none = 0, gate = 1, check = 2 };
enum class PortKind : uint8_t { scalar = 0, bus = 1, bundle = 2 };

uint8_t
directionCode(const PortDirection *dir)
{
  if (dir == PortDirection::input()) return 0;
  if (dir == PortDirection::output()) return 1;
  if (dir == PortDirection::tristate()) return 2;
  if (dir == PortDirection::bidirect()) return 3;
  if (dir == PortDirection::internal()) return 4;
  if (dir == PortDirection::ground()) return 5;
  if (dir == PortDirection::power()) return 6;
  if (dir == PortDirection::well()) return 7;
  return 8;
}

class LibWriter
{
public:
  LibWriter(LibertyLibrary *lib,
            Report *report) :
    lib_(lib),
    report_(report)
  {
  }

  void write(const char *path);

private:
  void writeLibrary();
  void writeUnit(const Unit *unit);
  void writeCell(LibertyCell *cell);
  void writePort(LibertyPort *port);
  void writePortAttrs(LibertyPort *port);
  void writeFuncExpr(const FuncExpr *expr);
  void writeArcSet(TimingArcSet *set);
  void writeAttrs(TimingArcSet *set);
  void writeModel(const TimingModel *model);
  void writeTableModels(const TableModels *models);
  void writeTableModel(const TableModel *model);
  void writeTableRef(const TablePtr &table);
  void writeAxisRef(const TableAxisPtr &axis);
  void writeRiseFallMinMax(const LibertyPort *port);
  void writeMinMaxLimit(const LibertyPort *port,
                        void (LibertyPort::*getter)(const MinMax *, float &, bool &) const);
  void writePortRef(const LibertyPort *port);

  DbWriter w_;
  LibertyLibrary *lib_;
  Report *report_;
  std::map<const TableAxis *, uint32_t> axis_ids_;
  std::map<const Table *, uint32_t> table_ids_;
  // Arc sets that share a TimingArcAttrs share its models, and a model is
  // owned by exactly one attrs, so the model pointer pair identifies the
  // attrs group without needing access to the attrs pointer itself.
  std::map<std::pair<const TimingModel *, const TimingModel *>, uint32_t> attrs_ids_;
};

////////////////////////////////////////////////////////////////
// Interned references.
//
// An id equal to the next unused id means the definition follows inline. That
// keeps the file single-pass in both directions with no section offsets.

void
LibWriter::writePortRef(const LibertyPort *port)
{
  // A presence flag rather than a sentinel id, because any 32-bit value is a
  // legal string table index.
  if (port == nullptr) {
    w_.boolean(false);
    return;
  }
  w_.boolean(true);
  w_.str(port->name());
}

void
LibWriter::writeAxisRef(const TableAxisPtr &axis)
{
  if (!axis) {
    w_.u32(kNullId);
    return;
  }
  auto it = axis_ids_.find(axis.get());
  if (it != axis_ids_.end()) {
    w_.u32(it->second);
    return;
  }
  uint32_t id = static_cast<uint32_t>(axis_ids_.size());
  axis_ids_[axis.get()] = id;
  w_.u32(id);
  w_.u8(static_cast<uint8_t>(axis->variable()));
  w_.floats(axis->values());
}

void
LibWriter::writeTableRef(const TablePtr &table)
{
  if (!table) {
    w_.u32(kNullId);
    return;
  }
  auto it = table_ids_.find(table.get());
  if (it != table_ids_.end()) {
    w_.u32(it->second);
    return;
  }
  uint32_t id = static_cast<uint32_t>(table_ids_.size());
  table_ids_[table.get()] = id;
  w_.u32(id);

  int order = table->order();
  w_.u8(static_cast<uint8_t>(order));
  writeAxisRef(table->axis1ptr());
  writeAxisRef(table->axis2ptr());
  writeAxisRef(table->axis3ptr());

  if (order == 0)
    w_.f32(table->value(size_t(0), size_t(0), size_t(0)));
  else if (order == 1) {
    const FloatSeq *values = table->values();
    static const FloatSeq empty;
    w_.floats(values ? *values : empty);
  }
  else {
    // Orders 2 and 3 share one FloatTable; order 3 flattens (axis1, axis2)
    // into the row index, so storing rows verbatim preserves both layouts.
    const FloatTable *values = table->values3();
    uint32_t rows = values ? static_cast<uint32_t>(values->size()) : 0;
    w_.u32(rows);
    for (uint32_t i = 0; i < rows; i++)
      w_.floats((*values)[i]);
  }
}

////////////////////////////////////////////////////////////////
// Timing models.

void
LibWriter::writeTableModel(const TableModel *model)
{
  if (model == nullptr) {
    w_.boolean(false);
    return;
  }
  w_.boolean(true);
  writeTableRef(model->table());
  TableTemplate *tmpl = model->tblTemplate();
  if (tmpl) {
    w_.boolean(true);
    w_.str(tmpl->name());
    w_.u8(static_cast<uint8_t>(tmpl->type()));
  }
  else
    w_.boolean(false);
  w_.u8(static_cast<uint8_t>(model->scaleFactorType()));
  w_.u8(static_cast<uint8_t>(model->rfIndex()));
}

void
LibWriter::writeTableModels(const TableModels *models)
{
  if (models == nullptr) {
    w_.boolean(false);
    return;
  }
  w_.boolean(true);
  // Only the NLDM model is stored. sigma/std_dev/mean_shift/skewness are LVF
  // and are dropped with CCS; see kFlagHasLvf.
  writeTableModel(models->model());
}

void
LibWriter::writeModel(const TimingModel *model)
{
  if (const GateTableModel *gate = dynamic_cast<const GateTableModel *>(model)) {
    w_.u8(static_cast<uint8_t>(ModelKind::gate));
    writeTableModels(gate->delayModels());
    writeTableModels(gate->slewModels());
  }
  else if (const CheckTableModel *check = dynamic_cast<const CheckTableModel *>(model)) {
    w_.u8(static_cast<uint8_t>(ModelKind::check));
    writeTableModels(check->checkModels());
  }
  else
    w_.u8(static_cast<uint8_t>(ModelKind::none));
}

////////////////////////////////////////////////////////////////

void
LibWriter::writeFuncExpr(const FuncExpr *expr)
{
  if (expr == nullptr) {
    w_.u8(0xFF);
    return;
  }
  w_.u8(static_cast<uint8_t>(expr->op()));
  if (expr->op() == FuncExpr::Op::port) {
    writePortRef(expr->port());
    return;
  }
  writeFuncExpr(expr->left());
  writeFuncExpr(expr->right());
}

void
LibWriter::writeAttrs(TimingArcSet *set)
{
  const TimingModel *m0 = set->model(RiseFall::rise());
  const TimingModel *m1 = set->model(RiseFall::fall());
  bool internable = (m0 != nullptr || m1 != nullptr);

  if (internable) {
    auto key = std::make_pair(m0, m1);
    auto it = attrs_ids_.find(key);
    if (it != attrs_ids_.end()) {
      w_.u32(it->second);
      return;
    }
    uint32_t id = static_cast<uint32_t>(attrs_ids_.size());
    attrs_ids_[key] = id;
    w_.u32(id);
  }
  else
    w_.u32(kNullId);

  w_.u8(static_cast<uint8_t>(set->timingType()));
  w_.u8(static_cast<uint8_t>(set->sense()));
  writeFuncExpr(set->cond());
  w_.str(set->sdfCondStart());
  w_.str(set->sdfCondEnd());
  w_.str(set->modeName());
  w_.str(set->modeValue());
  writeModel(m0);
  writeModel(m1);
}

void
LibWriter::writeArcSet(TimingArcSet *set)
{
  writePortRef(set->from());
  writePortRef(set->to());
  writePortRef(set->relatedOut());
  w_.str(set->role()->to_string());
  writeAttrs(set);

  const TimingArcSeq &arcs = set->arcs();
  w_.u32(static_cast<uint32_t>(arcs.size()));
  for (const TimingArc *arc : arcs) {
    w_.str(arc->fromEdge()->to_string());
    w_.str(arc->toEdge()->to_string());
    // An arc's model is always one of the two attrs models, so store the slot
    // rather than the model, which keeps the sharing intact on reload.
    const TimingModel *model = arc->model();
    uint8_t slot = 0xFF;
    if (model != nullptr && model == set->model(RiseFall::rise()))
      slot = 0;
    else if (model != nullptr && model == set->model(RiseFall::fall()))
      slot = 1;
    w_.u8(slot);
  }
}

////////////////////////////////////////////////////////////////

void
LibWriter::writeRiseFallMinMax(const LibertyPort *port)
{
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *mm : MinMax::range()) {
      float value;
      bool exists;
      port->capacitance(rf, mm, value, exists);
      w_.boolean(exists);
      w_.f32(exists ? value : 0.0F);
    }
  }
}

void
LibWriter::writeMinMaxLimit(const LibertyPort *port,
                            void (LibertyPort::*getter)(const MinMax *, float &,
                                                        bool &) const)
{
  for (const MinMax *mm : MinMax::range()) {
    float value;
    bool exists;
    (port->*getter)(mm, value, exists);
    w_.boolean(exists);
    w_.f32(exists ? value : 0.0F);
  }
}

void
LibWriter::writePortAttrs(LibertyPort *port)
{
  w_.u8(directionCode(port->direction()));
  writeRiseFallMinMax(port);
  writeMinMaxLimit(port, &LibertyPort::slewLimit);
  writeMinMaxLimit(port, &LibertyPort::capacitanceLimit);
  writeMinMaxLimit(port, &LibertyPort::fanoutLimit);

  float value;
  bool exists;
  port->fanoutLoad(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);

  port->minPeriod(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);

  for (const RiseFall *rf : RiseFall::range()) {
    port->minPulseWidth(rf, value, exists);
    w_.boolean(exists);
    w_.f32(exists ? value : 0.0F);
  }

  w_.boolean(port->isClock());
  w_.boolean(port->isClockGateClock());
  w_.boolean(port->isClockGateEnable());
  w_.boolean(port->isClockGateOut());
  w_.boolean(port->isPllFeedback());
  w_.boolean(port->isolationCellData());
  w_.boolean(port->isolationCellEnable());
  w_.boolean(port->levelShifterData());
  w_.boolean(port->isSwitch());
  w_.boolean(port->isPad());

  w_.u8(static_cast<uint8_t>(port->pwrGndType()));
  w_.str(port->voltageName());
  w_.u8(static_cast<uint8_t>(port->scanSignalType()));

  const RiseFall *trigger = port->pulseClkTrigger();
  const RiseFall *sense = port->pulseClkSense();
  w_.boolean(trigger != nullptr);
  if (trigger) {
    w_.u8(static_cast<uint8_t>(trigger->index()));
    w_.u8(static_cast<uint8_t>(sense ? sense->index() : 0));
  }

  writeFuncExpr(port->function());
  writeFuncExpr(port->tristateEnable());
  writePortRef(port->relatedPowerPort());
  writePortRef(port->relatedGroundPort());
}

void
LibWriter::writePort(LibertyPort *port)
{
  w_.str(port->name());
  if (port->isBus()) {
    w_.u8(static_cast<uint8_t>(PortKind::bus));
    w_.i32(port->fromIndex());
    w_.i32(port->toIndex());
    // LibertyCell exposes no iterator over its local bus declarations, so the
    // declaration is stored inline and recreated on demand at load.
    BusDcl *dcl = port->busDcl();
    w_.boolean(dcl != nullptr);
    if (dcl) {
      w_.str(dcl->name());
      w_.i32(dcl->from());
      w_.i32(dcl->to());
    }
  }
  else if (port->isBundle()) {
    w_.u8(static_cast<uint8_t>(PortKind::bundle));
    LibertyPortMemberIterator member_iter(port);
    std::vector<LibertyPort *> members;
    while (member_iter.hasNext())
      members.push_back(member_iter.next());
    w_.u32(static_cast<uint32_t>(members.size()));
    for (LibertyPort *member : members)
      w_.str(member->name());
  }
  else
    w_.u8(static_cast<uint8_t>(PortKind::scalar));
}

////////////////////////////////////////////////////////////////

void
LibWriter::writeCell(LibertyCell *cell)
{
  w_.str(cell->name());
  w_.str(cell->filename());
  w_.f32(cell->area());
  w_.boolean(cell->dontUse());
  w_.boolean(cell->isMacro());
  w_.boolean(cell->isMemory());
  w_.boolean(cell->isPad());
  w_.boolean(cell->isClockCell());
  w_.boolean(cell->isLevelShifter());
  w_.u8(static_cast<uint8_t>(cell->levelShifterType()));
  w_.boolean(cell->isIsolationCell());
  w_.boolean(cell->alwaysOn());
  w_.u8(static_cast<uint8_t>(cell->switchCellType()));
  w_.boolean(cell->interfaceTiming());
  w_.str(cell->footprint());
  w_.str(cell->userFunctionClass());
  w_.f32(cell->ocvArcDepth());
  // inferLatchRoles() only runs when this is set, and it is what lets
  // makeLatchEnables() rebuild latch_enables_ from the reloaded roles.
  w_.boolean(cell->hasInferedRegTimingArcs());

  ClockGateType cg = ClockGateType::none;
  if (cell->isClockGateLatchPosedge()) cg = ClockGateType::latch_posedge;
  else if (cell->isClockGateLatchNegedge()) cg = ClockGateType::latch_negedge;
  else if (cell->isClockGateOther()) cg = ClockGateType::other;
  w_.u8(static_cast<uint8_t>(cg));

  ScaleFactors *sf = cell->scaleFactors();
  w_.boolean(sf != nullptr);
  if (sf)
    w_.str(sf->name());

  // Port creation order drives pin_index_ and therefore the per-instance pin
  // array layout, so the ordered port list is written, not the name map.
  std::vector<LibertyPort *> ports;
  LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext())
    ports.push_back(port_iter.next());

  w_.u32(static_cast<uint32_t>(ports.size()));
  for (LibertyPort *port : ports)
    writePort(port);

  // Attributes come after every port exists, so functions can reference ports
  // declared later in the cell. Containers are written before bits because
  // setting a bus attribute propagates to its members, and the bit's own
  // value has to be the one that lands last.
  std::vector<LibertyPort *> bits;
  LibertyCellPortBitIterator bit_iter(cell);
  while (bit_iter.hasNext())
    bits.push_back(bit_iter.next());

  w_.u32(static_cast<uint32_t>(ports.size() + bits.size()));
  for (LibertyPort *port : ports) {
    w_.str(port->name());
    writePortAttrs(port);
  }
  for (LibertyPort *port : bits) {
    w_.str(port->name());
    writePortAttrs(port);
  }

  const SequentialSeq &seqs = cell->sequentials();
  w_.u32(static_cast<uint32_t>(seqs.size()));
  for (const Sequential &seq : seqs) {
    w_.boolean(seq.isRegister());
    writeFuncExpr(seq.clock());
    writeFuncExpr(seq.data());
    writeFuncExpr(seq.clear());
    writeFuncExpr(seq.preset());
    w_.u8(static_cast<uint8_t>(seq.clearPresetOutput()));
    w_.u8(static_cast<uint8_t>(seq.clearPresetOutputInv()));
    writePortRef(seq.output());
    writePortRef(seq.outputInv());
  }

  // Statetables are the other source of isSequential(): sky130's clock gating
  // latches are described this way rather than with a latch group.
  const Statetable *statetable = cell->statetable();
  w_.boolean(statetable != nullptr);
  if (statetable) {
    const LibertyPortSeq &inputs = statetable->inputPorts();
    w_.u32(static_cast<uint32_t>(inputs.size()));
    for (const LibertyPort *port : inputs)
      writePortRef(port);
    const LibertyPortSeq &internals = statetable->internalPorts();
    w_.u32(static_cast<uint32_t>(internals.size()));
    for (const LibertyPort *port : internals)
      writePortRef(port);

    const StatetableRows &rows = statetable->table();
    w_.u32(static_cast<uint32_t>(rows.size()));
    for (const StatetableRow &row : rows) {
      w_.u32(static_cast<uint32_t>(row.inputValues().size()));
      for (StateInputValue v : row.inputValues())
        w_.u8(static_cast<uint8_t>(v));
      w_.u32(static_cast<uint32_t>(row.currentValues().size()));
      for (StateInternalValue v : row.currentValues())
        w_.u8(static_cast<uint8_t>(v));
      w_.u32(static_cast<uint32_t>(row.nextValues().size()));
      for (StateInternalValue v : row.nextValues())
        w_.u8(static_cast<uint8_t>(v));
    }
  }

  const GeneratedClockSeq &gen_clks = cell->generatedClocks();
  w_.u32(static_cast<uint32_t>(gen_clks.size()));
  for (const GeneratedClock *gc : gen_clks) {
    w_.str(gc->name());
    w_.str(gc->clockPin());
    w_.str(gc->masterPin());
    w_.i32(gc->dividedBy());
    w_.i32(gc->multipliedBy());
    w_.f32(gc->dutyCycle());
    w_.boolean(gc->invert());
    const IntSeq *edges = gc->edges();
    w_.u32(edges ? static_cast<uint32_t>(edges->size()) : 0);
    if (edges)
      for (int edge : *edges)
        w_.i32(edge);
    const FloatSeq *shifts = gc->edgeShifts();
    static const FloatSeq empty;
    w_.floats(shifts ? *shifts : empty);
  }

  // Arc set order defines TimingArcSet::index_, which makeSceneMap uses to
  // align corners, so the sequence is written verbatim.
  const TimingArcSetSeq &arc_sets = cell->timingArcSets();
  w_.u32(static_cast<uint32_t>(arc_sets.size()));
  for (TimingArcSet *set : arc_sets)
    writeArcSet(set);
}

////////////////////////////////////////////////////////////////

void
LibWriter::writeUnit(const Unit *unit)
{
  w_.f32(unit->scale());
  w_.str(unit->suffix());
  w_.i32(unit->digits());
}

void
LibWriter::writeLibrary()
{
  w_.str(lib_->name());
  w_.str(lib_->filename());
  w_.u8(static_cast<uint8_t>(lib_->delayModelType()));
  w_.u8(static_cast<uint8_t>(lib_->busBrktLeft()));
  w_.u8(static_cast<uint8_t>(lib_->busBrktRight()));

  const Units *units = lib_->units();
  writeUnit(units->timeUnit());
  writeUnit(units->capacitanceUnit());
  writeUnit(units->voltageUnit());
  writeUnit(units->resistanceUnit());
  writeUnit(units->currentUnit());
  writeUnit(units->powerUnit());
  writeUnit(units->distanceUnit());
  writeUnit(units->scalarUnit());

  w_.f32(lib_->nominalProcess());
  w_.f32(lib_->nominalVoltage());
  w_.f32(lib_->nominalTemperature());
  w_.f32(lib_->defaultInputPinCap());
  w_.f32(lib_->defaultOutputPinCap());
  w_.f32(lib_->defaultBidirectPinCap());

  float value;
  bool exists;
  for (const RiseFall *rf : RiseFall::range()) {
    lib_->defaultIntrinsic(rf, value, exists);
    w_.boolean(exists);
    w_.f32(exists ? value : 0.0F);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    lib_->defaultBidirectPinRes(rf, value, exists);
    w_.boolean(exists);
    w_.f32(exists ? value : 0.0F);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    lib_->defaultOutputPinRes(rf, value, exists);
    w_.boolean(exists);
    w_.f32(exists ? value : 0.0F);
  }

  lib_->defaultFanoutLoad(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);
  lib_->defaultMaxCapacitance(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);
  lib_->defaultMaxFanout(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);
  lib_->defaultMaxSlew(value, exists);
  w_.boolean(exists);
  w_.f32(exists ? value : 0.0F);

  for (const RiseFall *rf : RiseFall::range()) w_.f32(lib_->inputThreshold(rf));
  for (const RiseFall *rf : RiseFall::range()) w_.f32(lib_->outputThreshold(rf));
  for (const RiseFall *rf : RiseFall::range()) w_.f32(lib_->slewLowerThreshold(rf));
  for (const RiseFall *rf : RiseFall::range()) w_.f32(lib_->slewUpperThreshold(rf));
  w_.f32(lib_->slewDerateFromLibrary());
  w_.f32(lib_->ocvArcDepth());

  BusDclSeq bus_dcls = lib_->busDcls();
  w_.u32(static_cast<uint32_t>(bus_dcls.size()));
  for (BusDcl *dcl : bus_dcls) {
    w_.str(dcl->name());
    w_.i32(dcl->from());
    w_.i32(dcl->to());
  }

  TableTemplateSeq templates = lib_->tableTemplates();
  w_.u32(static_cast<uint32_t>(templates.size()));
  for (TableTemplate *tmpl : templates) {
    w_.str(tmpl->name());
    w_.u8(static_cast<uint8_t>(tmpl->type()));
    writeAxisRef(tmpl->axis1ptr());
    writeAxisRef(tmpl->axis2ptr());
    writeAxisRef(tmpl->axis3ptr());
  }

  // Only the default operating conditions and scale factors are stored:
  // LibertyLibrary exposes no iterator over the named maps, so non-default
  // entries would need a new accessor. They only matter under
  // set_operating_conditions with a named corner.
  OperatingConditions *op_cond = lib_->defaultOperatingConditions();
  w_.boolean(op_cond != nullptr);
  if (op_cond) {
    w_.str(op_cond->name());
    w_.f32(op_cond->process());
    w_.f32(op_cond->voltage());
    w_.f32(op_cond->temperature());
    w_.u8(static_cast<uint8_t>(op_cond->wireloadTree()));
  }

  ScaleFactors *scales = lib_->scaleFactors();
  w_.boolean(scales != nullptr);
  if (scales) {
    w_.str(scales->name());
    for (int type = 0; type < scale_factor_type_count; type++) {
      for (int pvt = 0; pvt < scale_factor_pvt_count; pvt++) {
        for (const RiseFall *rf : RiseFall::range())
          w_.f32(scales->scale(static_cast<ScaleFactorType>(type),
                               static_cast<ScaleFactorPvt>(pvt), rf));
      }
    }
  }

  LibertyCellIterator cell_iter(lib_);
  std::vector<LibertyCell *> cells;
  while (cell_iter.hasNext())
    cells.push_back(cell_iter.next());
  w_.u32(static_cast<uint32_t>(cells.size()));
  for (LibertyCell *cell : cells)
    writeCell(cell);
}

void
LibWriter::write(const char *path)
{
  writeLibrary();

  FILE *f = fopen(path, "wb");
  if (f == nullptr)
    report_->error(1352, "cannot open {} for writing.", path);

  // The string table is emitted after the body because ids are assigned on
  // demand while serializing; the header carries both section sizes.
  DbWriter strings;
  for (const std::string &s : w_.strings()) {
    strings.u32(static_cast<uint32_t>(s.size()));
    for (char c : s)
      strings.u8(static_cast<uint8_t>(c));
  }

  LibDbHeader hdr{};
  std::memcpy(hdr.magic, kLibDbMagic, sizeof hdr.magic);
  hdr.version = kLibDbVersion;
  hdr.flags = 0;
  hdr.infer_latches = 0;
  hdr.pointer_size = sizeof(void *);
  hdr.string_bytes = strings.size();
  hdr.body_bytes = w_.size();

  bool ok = fwrite(&hdr, sizeof hdr, 1, f) == 1;
  uint32_t count = static_cast<uint32_t>(w_.strings().size());
  ok = ok && fwrite(&count, sizeof count, 1, f) == 1;
  if (ok && strings.size())
    ok = fwrite(strings.bytes().data(), strings.size(), 1, f) == 1;
  if (ok && w_.size())
    ok = fwrite(w_.bytes().data(), w_.size(), 1, f) == 1;
  fclose(f);

  if (!ok)
    report_->error(1353, "error writing {}.", path);
}

} // namespace

void
writeLibDbFile(LibertyLibrary *library,
               std::string_view filename,
               Report *report)
{
  if (library == nullptr)
    report->error(1354, "no liberty library to write.");
  std::string path(filename);
  LibWriter writer(library, report);
  writer.write(path.c_str());
}

} // namespace sta
