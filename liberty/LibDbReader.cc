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

// read_lib_db: rebuild one NLDM LibertyLibrary from a .libdb cache.
//
// File layout: [header][string_count][string table][body]
// Body field order must match LibWriter in LibDbWriter.cc.
// Shared axes/tables/attrs: first use reads id + full data; later uses reuse by id.
// Strings: body stores an index; the string table holds the text once.

#include "LibDb.hh"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "ConcreteLibrary.hh"
#include "Debug.hh"
#include "FuncExpr.hh"
#include "Liberty.hh"
#include "LibertyBuilder.hh"
#include "Network.hh"
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

static PortDirection *
directionFromCode(uint8_t code)
{
  // Inverse of directionCode() in the writer.
  switch (code) {
  case 0: return PortDirection::input();
  case 1: return PortDirection::output();
  case 2: return PortDirection::tristate();
  case 3: return PortDirection::bidirect();
  case 4: return PortDirection::internal();
  case 5: return PortDirection::ground();
  case 6: return PortDirection::power();
  case 7: return PortDirection::well();
  default: return PortDirection::unknown();
  }
}

// Inverse of LibWriter: read body fields in the same order and build objects.
class LibLoader
{
public:
  LibLoader(DbReader &r,
            std::string_view filename,
            Network *network) :
    r_(r),
    filename_(filename),
    network_(network),
    report_(network->report()),
    debug_(network->debug()),
    builder_(network->debug(), network->report())
  {
  }

  LibertyLibrary *read();

private:
  void readUnit(Unit *unit);
  void readCell();
  void readPort(LibertyCell *cell);
  void readPortAttrs(LibertyCell *cell);
  FuncExpr *readFuncExpr(LibertyCell *cell);
  void readArcSet(LibertyCell *cell);
  TimingArcAttrsPtr readAttrs(LibertyCell *cell);
  TimingModel *readModel(LibertyCell *cell);
  TableModels *readTableModels();
  TableModel *readTableModel();
  TablePtr readTableRef();
  TableAxisPtr readAxisRef();
  LibertyPort *readPortRef(LibertyCell *cell);

  DbReader &r_;                 // bookmark over body bytes + string list
  std::string filename_;
  Network *network_;
  Report *report_;
  Debug *debug_;
  LibertyBuilder builder_;      // creates cells/ports like the liberty parser
  LibertyLibrary *lib_{nullptr};

  // Shared objects seen so far (index = number written in the file).
  std::vector<TableAxisPtr> axes_;
  std::vector<TablePtr> tables_;
  std::vector<TimingArcAttrsPtr> attrs_;
};

////////////////////////////////////////////////////////////////
// Shared objects: first time we see a number, the full data follows and we
// remember it. Next time that number appears, reuse the remembered object.

LibertyPort *
LibLoader::readPortRef(LibertyCell *cell)
{
  // Optional port by name (flag + string), not a raw pointer.
  if (!r_.boolean())
    return nullptr;
  const std::string &name = r_.str();
  return cell ? cell->findLibertyPort(name) : nullptr;
}

TableAxisPtr
LibLoader::readAxisRef()
{
  uint32_t id = r_.u32();
  if (id == lib_db_id_null)
    return nullptr;
  if (id < axes_.size())
    return axes_[id];  // seen before

  // First time: full axis data is next; save it under this number.
  TableAxisVariable var = static_cast<TableAxisVariable>(r_.u8());
  FloatSeq values = r_.floats();
  TableAxisPtr axis = std::make_shared<TableAxis>(var, std::move(values));
  axes_.push_back(axis);
  return axis;
}

TablePtr
LibLoader::readTableRef()
{
  uint32_t id = r_.u32();
  if (id == lib_db_id_null)
    return nullptr;
  if (id < tables_.size())
    return tables_[id];  // seen before

  // First time: full table data is next.
  // order 0/1/2/3 picks scalar / 1D / 2D / 3D Table ctor.
  int order = r_.u8();
  TableAxisPtr axis1 = readAxisRef();
  TableAxisPtr axis2 = readAxisRef();
  TableAxisPtr axis3 = readAxisRef();

  TablePtr table;
  if (order == 0)
    table = std::make_shared<Table>(r_.f32());
  else if (order == 1)
    table = std::make_shared<Table>(r_.floats(), axis1);
  else {
    uint32_t rows = r_.u32();
    FloatTable values;
    values.reserve(rows);
    for (uint32_t i = 0; i < rows; i++)
      values.push_back(r_.floats());
    if (order == 2)
      table = std::make_shared<Table>(std::move(values), axis1, axis2);
    else
      table = std::make_shared<Table>(std::move(values), axis1, axis2, axis3);
  }
  tables_.push_back(table);
  return table;
}

////////////////////////////////////////////////////////////////

TableModel *
LibLoader::readTableModel()
{
  if (!r_.boolean())
    return nullptr;
  TablePtr table = readTableRef();
  TableTemplate *tmpl = nullptr;
  if (r_.boolean()) {
    const std::string &name = r_.str();
    TableTemplateType type = static_cast<TableTemplateType>(r_.u8());
    tmpl = lib_->findTableTemplate(name, type);
  }
  ScaleFactorType sf_type = static_cast<ScaleFactorType>(r_.u8());
  const RiseFall *rf = RiseFall::find(static_cast<size_t>(r_.u8()));
  return new TableModel(table, tmpl, sf_type, rf);
}

TableModels *
LibLoader::readTableModels()
{
  if (!r_.boolean())
    return nullptr;
  return new TableModels(readTableModel());
}

TimingModel *
LibLoader::readModel(LibertyCell *cell)
{
  // One byte kind, then delay/slew (gate) or check tables.
  LibDbModelKind kind = static_cast<LibDbModelKind>(r_.u8());
  switch (kind) {
  case LibDbModelKind::gate: {
    TableModels *delay = readTableModels();
    TableModels *slew = readTableModels();
    return new GateTableModel(cell, delay, slew);
  }
  case LibDbModelKind::check: {
    TableModels *check = readTableModels();
    return new CheckTableModel(cell, check);
  }
  default:
    return nullptr;
  }
}

////////////////////////////////////////////////////////////////
// Mirror writeFuncExpr. not_ still reads a dummy right child to stay aligned.

FuncExpr *
LibLoader::readFuncExpr(LibertyCell *cell)
{
  uint8_t op_code = r_.u8();
  if (op_code == 0xFF)
    return nullptr;
  FuncExpr::Op op = static_cast<FuncExpr::Op>(op_code);
  switch (op) {
  case FuncExpr::Op::port: {
    LibertyPort *port = readPortRef(cell);
    return port ? FuncExpr::makePort(port) : nullptr;
  }
  case FuncExpr::Op::not_: {
    FuncExpr *left = readFuncExpr(cell);
    // Writer always emits left+right; discard the unused right for unary not.
    FuncExpr *right = readFuncExpr(cell);
    (void) right;
    return left ? FuncExpr::makeNot(left) : nullptr;
  }
  case FuncExpr::Op::or_: {
    FuncExpr *left = readFuncExpr(cell);
    FuncExpr *right = readFuncExpr(cell);
    return (left && right) ? FuncExpr::makeOr(left, right) : nullptr;
  }
  case FuncExpr::Op::and_: {
    FuncExpr *left = readFuncExpr(cell);
    FuncExpr *right = readFuncExpr(cell);
    return (left && right) ? FuncExpr::makeAnd(left, right) : nullptr;
  }
  case FuncExpr::Op::xor_: {
    FuncExpr *left = readFuncExpr(cell);
    FuncExpr *right = readFuncExpr(cell);
    return (left && right) ? FuncExpr::makeXor(left, right) : nullptr;
  }
  case FuncExpr::Op::one:
    readFuncExpr(cell);
    readFuncExpr(cell);
    return FuncExpr::makeOne();
  case FuncExpr::Op::zero:
    readFuncExpr(cell);
    readFuncExpr(cell);
    return FuncExpr::makeZero();
  }
  return nullptr;
}

TimingArcAttrsPtr
LibLoader::readAttrs(LibertyCell *cell)
{
  // Reuse by id, or build TimingArcAttrs and store if id was new.
  uint32_t id = r_.u32();
  if (id != lib_db_id_null && id < attrs_.size())
    return attrs_[id];

  TimingArcAttrsPtr attrs = std::make_shared<TimingArcAttrs>();
  attrs->setTimingType(static_cast<TimingType>(r_.u8()));
  attrs->setTimingSense(static_cast<TimingSense>(r_.u8()));
  FuncExpr *cond = readFuncExpr(cell);
  if (cond)
    attrs->setCond(cond);
  attrs->setSdfCondStart(r_.str());
  attrs->setSdfCondEnd(r_.str());
  attrs->setModeName(r_.str());
  attrs->setModeValue(r_.str());
  for (const RiseFall *rf : RiseFall::range()) {
    TimingModel *model = readModel(cell);
    if (model)
      attrs->setModel(rf, model);
  }
  if (id != lib_db_id_null)
    attrs_.push_back(attrs);
  return attrs;
}

void
LibLoader::readArcSet(LibertyCell *cell)
{
  LibertyPort *from = readPortRef(cell);
  LibertyPort *to = readPortRef(cell);
  LibertyPort *related_out = readPortRef(cell);
  const TimingRole *role = TimingRole::find(r_.str().c_str());
  TimingArcAttrsPtr attrs = readAttrs(cell);

  // Replay resolved arcs (do not re-infer via LibertyBuilder).
  TimingArcSet *set = cell->makeTimingArcSet(from, to, related_out, role, attrs);

  // slot 0/1 = rise/fall model from attrs; 0xFF = no model.
  uint32_t arc_count = r_.u32();
  for (uint32_t i = 0; i < arc_count; i++) {
    const Transition *from_rf = Transition::find(r_.str());
    const Transition *to_rf = Transition::find(r_.str());
    uint8_t slot = r_.u8();
    TimingModel *model = nullptr;
    if (slot == 0)
      model = attrs->model(RiseFall::rise());
    else if (slot == 1)
      model = attrs->model(RiseFall::fall());
    new TimingArc(set, from_rf, to_rf, model);
  }
}

////////////////////////////////////////////////////////////////

void
LibLoader::readPort(LibertyCell *cell)
{
  // Create port shell (scalar/bus/bundle); attrs come in a later pass.
  const std::string name = r_.str();
  LibDbPortKind kind = static_cast<LibDbPortKind>(r_.u8());
  if (kind == LibDbPortKind::bus) {
    int from = r_.i32();
    int to = r_.i32();
    BusDcl *dcl = nullptr;
    if (r_.boolean()) {
      const std::string dcl_name = r_.str();
      int dcl_from = r_.i32();
      int dcl_to = r_.i32();
      dcl = cell->findBusDcl(dcl_name);
      if (dcl == nullptr)
        dcl = cell->makeBusDcl(dcl_name, dcl_from, dcl_to);
    }
    builder_.makeBusPort(cell, name, from, to, dcl);
  }
  else if (kind == LibDbPortKind::bundle) {
    uint32_t count = r_.u32();
    ConcretePortSeq *members = new ConcretePortSeq;
    for (uint32_t i = 0; i < count; i++) {
      LibertyPort *member = cell->findLibertyPort(r_.str());
      if (member)
        members->push_back(member);
    }
    builder_.makeBundlePort(cell, name, members);
  }
  else
    builder_.makePort(cell, name);
}

void
LibLoader::readPortAttrs(LibertyCell *cell)
{
  // Same order as writePortAttrs; skip set* when exists flag is false.
  LibertyPort *port = cell->findLibertyPort(r_.str());

  PortDirection *dir = directionFromCode(r_.u8());
  if (port)
    port->setDirection(dir);

  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *mm : MinMax::range()) {
      bool exists = r_.boolean();
      float value = r_.f32();
      if (port && exists)
        port->setCapacitance(rf, mm, value);
    }
  }
  for (const MinMax *mm : MinMax::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (port && exists)
      port->setSlewLimit(value, mm);
  }
  for (const MinMax *mm : MinMax::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (port && exists)
      port->setCapacitanceLimit(value, mm);
  }
  for (const MinMax *mm : MinMax::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (port && exists)
      port->setFanoutLimit(value, mm);
  }

  bool exists = r_.boolean();
  float value = r_.f32();
  if (port && exists)
    port->setFanoutLoad(value);

  exists = r_.boolean();
  value = r_.f32();
  if (port && exists)
    port->setMinPeriod(value);

  for (const RiseFall *rf : RiseFall::range()) {
    exists = r_.boolean();
    value = r_.f32();
    if (port && exists)
      port->setMinPulseWidth(rf, value);
  }

  bool is_clock = r_.boolean();
  bool cg_clk = r_.boolean();
  bool cg_enable = r_.boolean();
  bool cg_out = r_.boolean();
  bool pll_feedback = r_.boolean();
  bool iso_data = r_.boolean();
  bool iso_enable = r_.boolean();
  bool ls_data = r_.boolean();
  bool is_switch = r_.boolean();
  bool is_pad = r_.boolean();
  // Cell-level bits required by isClockGate() (same as liberty reader).
  if (cg_clk)
    cell->setHasClkGateClkPin();
  if (cg_enable)
    cell->setHasClkGateEnablePin();
  if (port) {
    port->setIsClock(is_clock);
    port->setIsClockGateClock(cg_clk);
    port->setIsClockGateEnable(cg_enable);
    port->setIsClockGateOut(cg_out);
    port->setIsPllFeedback(pll_feedback);
    port->setIsolationCellData(iso_data);
    port->setIsolationCellEnable(iso_enable);
    port->setLevelShifterData(ls_data);
    port->setIsSwitch(is_switch);
    port->setIsPad(is_pad);
  }

  PwrGndType pg = static_cast<PwrGndType>(r_.u8());
  const std::string voltage_name = r_.str();
  ScanSignalType scan = static_cast<ScanSignalType>(r_.u8());
  if (port) {
    port->setPwrGndType(pg);
    port->setVoltageName(voltage_name);
    port->setScanSignalType(scan);
  }

  if (r_.boolean()) {
    const RiseFall *trigger = RiseFall::find(static_cast<size_t>(r_.u8()));
    const RiseFall *sense = RiseFall::find(static_cast<size_t>(r_.u8()));
    if (port)
      port->setPulseClk(trigger, sense);
  }

  FuncExpr *func = readFuncExpr(cell);
  FuncExpr *tristate = readFuncExpr(cell);
  LibertyPort *related_power = readPortRef(cell);
  LibertyPort *related_ground = readPortRef(cell);
  if (port) {
    if (func)
      port->setFunction(func);
    if (tristate)
      port->setTristateEnable(tristate);
    if (related_power)
      port->setRelatedPowerPort(related_power);
    if (related_ground)
      port->setRelatedGroundPort(related_ground);
  }
}

////////////////////////////////////////////////////////////////

void
LibLoader::readCell()
{
  // Ports → port attrs → sequentials → statetable → gen clocks → arcs → finish(false).
  const std::string name = r_.str();
  const std::string filename = r_.str();
  LibertyCell *cell = builder_.makeCell(lib_, name, filename);

  cell->setArea(r_.f32());
  cell->setDontUse(r_.boolean());
  cell->setIsMacro(r_.boolean());
  cell->setIsMemory(r_.boolean());
  cell->setIsPad(r_.boolean());
  cell->setIsClockCell(r_.boolean());
  cell->setIsLevelShifter(r_.boolean());
  cell->setLevelShifterType(static_cast<LevelShifterType>(r_.u8()));
  cell->setIsIsolationCell(r_.boolean());
  cell->setAlwaysOn(r_.boolean());
  cell->setSwitchCellType(static_cast<SwitchCellType>(r_.u8()));
  cell->setInterfaceTiming(r_.boolean());
  cell->setFootprint(r_.str());
  cell->setUserFunctionClass(r_.str());
  cell->setOcvArcDepth(r_.f32());
  cell->setHasInferedRegTimingArcs(r_.boolean());
  cell->setClockGateType(static_cast<ClockGateType>(r_.u8()));

  if (r_.boolean())
    cell->setScaleFactors(lib_->findScaleFactors(r_.str()));

  uint32_t port_count = r_.u32();
  for (uint32_t i = 0; i < port_count; i++)
    readPort(cell);

  uint32_t attr_count = r_.u32();
  for (uint32_t i = 0; i < attr_count; i++)
    readPortAttrs(cell);

  uint32_t seq_count = r_.u32();
  for (uint32_t i = 0; i < seq_count; i++) {
    bool is_register = r_.boolean();
    FuncExpr *clk = readFuncExpr(cell);
    FuncExpr *data = readFuncExpr(cell);
    FuncExpr *clear = readFuncExpr(cell);
    FuncExpr *preset = readFuncExpr(cell);
    LogicValue clr_preset_out = static_cast<LogicValue>(r_.u8());
    LogicValue clr_preset_out_inv = static_cast<LogicValue>(r_.u8());
    LibertyPort *output = readPortRef(cell);
    LibertyPort *output_inv = readPortRef(cell);
    // Already split per bit; size-1 group. makeSequential owns the exprs.
    cell->makeSequential(1, is_register, clk, data, clear, preset,
                         clr_preset_out, clr_preset_out_inv, output, output_inv);
  }

  if (r_.boolean()) {
    LibertyPortSeq inputs;
    uint32_t input_count = r_.u32();
    for (uint32_t i = 0; i < input_count; i++) {
      LibertyPort *port = readPortRef(cell);
      if (port)
        inputs.push_back(port);
    }
    LibertyPortSeq internals;
    uint32_t internal_count = r_.u32();
    for (uint32_t i = 0; i < internal_count; i++) {
      LibertyPort *port = readPortRef(cell);
      if (port)
        internals.push_back(port);
    }

    StatetableRows rows;
    uint32_t row_count = r_.u32();
    for (uint32_t i = 0; i < row_count; i++) {
      StateInputValues input_values;
      uint32_t n = r_.u32();
      for (uint32_t j = 0; j < n; j++)
        input_values.push_back(static_cast<StateInputValue>(r_.u8()));
      StateInternalValues current_values;
      n = r_.u32();
      for (uint32_t j = 0; j < n; j++)
        current_values.push_back(static_cast<StateInternalValue>(r_.u8()));
      StateInternalValues next_values;
      n = r_.u32();
      for (uint32_t j = 0; j < n; j++)
        next_values.push_back(static_cast<StateInternalValue>(r_.u8()));
      rows.emplace_back(input_values, current_values, next_values);
    }
    cell->makeStatetable(inputs, internals, rows);
  }

  uint32_t gen_clk_count = r_.u32();
  for (uint32_t i = 0; i < gen_clk_count; i++) {
    const std::string gc_name = r_.str();
    const std::string clock_pin = r_.str();
    const std::string master_pin = r_.str();
    int divided_by = r_.i32();
    int multiplied_by = r_.i32();
    float duty_cycle = r_.f32();
    bool invert = r_.boolean();
    uint32_t edge_count = r_.u32();
    IntSeq *edges = edge_count ? new IntSeq : nullptr;
    for (uint32_t e = 0; e < edge_count; e++)
      edges->push_back(r_.i32());
    FloatSeq shift_values = r_.floats();
    FloatSeq *shifts = shift_values.empty() ? nullptr : new FloatSeq(shift_values);
    // makeGeneratedClock copies edges/shifts.
    cell->makeGeneratedClock(gc_name.c_str(), clock_pin.c_str(), master_pin.c_str(),
                             divided_by, multiplied_by, duty_cycle, invert,
                             edges, shifts);
    delete edges;
    delete shifts;
  }

  uint32_t arc_set_count = r_.u32();
  for (uint32_t i = 0; i < arc_set_count; i++)
    readArcSet(cell);

  // false = do not re-derive latch enables; arcs/roles already match the original liberty.
  cell->finish(false, report_, debug_);
}

void
LibLoader::readUnit(Unit *unit)
{
  unit->setScale(r_.f32());
  unit->setSuffix(r_.str().c_str());
  unit->setDigits(r_.i32());
}

LibertyLibrary *
LibLoader::read()
{
  // Same field order as LibWriter::writeLibrary — create empty library, then
  // fill units/defaults/templates, then each cell via readCell().
  const std::string name = r_.str();
  const std::string filename = r_.str();
  lib_ = network_->makeLibertyLibrary(name, filename);
  if (lib_ == nullptr)
    report_->error(1358, "cannot make liberty library for {}.", filename_);

  lib_->setDelayModelType(static_cast<DelayModelType>(r_.u8()));
  char brkt_left = static_cast<char>(r_.u8());
  char brkt_right = static_cast<char>(r_.u8());
  lib_->setBusBrkts(brkt_left, brkt_right);

  Units *units = lib_->units();
  readUnit(units->timeUnit());
  readUnit(units->capacitanceUnit());
  readUnit(units->voltageUnit());
  readUnit(units->resistanceUnit());
  readUnit(units->currentUnit());
  readUnit(units->powerUnit());
  readUnit(units->distanceUnit());
  readUnit(units->scalarUnit());

  lib_->setNominalProcess(r_.f32());
  lib_->setNominalVoltage(r_.f32());
  lib_->setNominalTemperature(r_.f32());
  lib_->setDefaultInputPinCap(r_.f32());
  lib_->setDefaultOutputPinCap(r_.f32());
  lib_->setDefaultBidirectPinCap(r_.f32());

  for (const RiseFall *rf : RiseFall::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (exists)
      lib_->setDefaultIntrinsic(rf, value);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (exists)
      lib_->setDefaultBidirectPinRes(rf, value);
  }
  for (const RiseFall *rf : RiseFall::range()) {
    bool exists = r_.boolean();
    float value = r_.f32();
    if (exists)
      lib_->setDefaultOutputPinRes(rf, value);
  }

  bool exists = r_.boolean();
  float value = r_.f32();
  if (exists)
    lib_->setDefaultFanoutLoad(value);
  exists = r_.boolean();
  value = r_.f32();
  if (exists)
    lib_->setDefaultMaxCapacitance(value);
  exists = r_.boolean();
  value = r_.f32();
  if (exists)
    lib_->setDefaultMaxFanout(value);
  exists = r_.boolean();
  value = r_.f32();
  if (exists)
    lib_->setDefaultMaxSlew(value);

  for (const RiseFall *rf : RiseFall::range()) lib_->setInputThreshold(rf, r_.f32());
  for (const RiseFall *rf : RiseFall::range()) lib_->setOutputThreshold(rf, r_.f32());
  for (const RiseFall *rf : RiseFall::range()) lib_->setSlewLowerThreshold(rf, r_.f32());
  for (const RiseFall *rf : RiseFall::range()) lib_->setSlewUpperThreshold(rf, r_.f32());
  lib_->setSlewDerateFromLibrary(r_.f32());
  lib_->setOcvArcDepth(r_.f32());

  uint32_t bus_dcl_count = r_.u32();
  for (uint32_t i = 0; i < bus_dcl_count; i++) {
    const std::string dcl_name = r_.str();
    int from = r_.i32();
    int to = r_.i32();
    lib_->makeBusDcl(dcl_name, from, to);
  }

  // Templates/axes must exist before cells that point at them.
  uint32_t template_count = r_.u32();
  for (uint32_t i = 0; i < template_count; i++) {
    const std::string tmpl_name = r_.str();
    TableTemplateType type = static_cast<TableTemplateType>(r_.u8());
    TableTemplate *tmpl = lib_->findTableTemplate(tmpl_name, type);
    if (tmpl == nullptr)
      tmpl = lib_->makeTableTemplate(tmpl_name, type);
    TableAxisPtr axis1 = readAxisRef();
    TableAxisPtr axis2 = readAxisRef();
    TableAxisPtr axis3 = readAxisRef();
    if (tmpl) {
      tmpl->setAxis1(axis1);
      tmpl->setAxis2(axis2);
      tmpl->setAxis3(axis3);
    }
  }

  if (r_.boolean()) {
    OperatingConditions *op_cond = lib_->makeOperatingConditions(r_.str());
    op_cond->setProcess(r_.f32());
    op_cond->setVoltage(r_.f32());
    op_cond->setTemperature(r_.f32());
    op_cond->setWireloadTree(static_cast<WireloadTree>(r_.u8()));
    lib_->setDefaultOperatingConditions(op_cond);
  }

  if (r_.boolean()) {
    ScaleFactors *scales = lib_->makeScaleFactors(r_.str());
    for (int type = 0; type < scale_factor_type_count; type++) {
      for (int pvt = 0; pvt < scale_factor_pvt_count; pvt++) {
        for (const RiseFall *rf : RiseFall::range())
          scales->setScale(static_cast<ScaleFactorType>(type),
                           static_cast<ScaleFactorPvt>(pvt), rf, r_.f32());
      }
    }
    lib_->setScaleFactors(scales);
  }

  uint32_t cell_count = r_.u32();
  for (uint32_t i = 0; i < cell_count; i++)
    readCell();

  // DbReader sets this if we tried to read past the end of the body.
  if (r_.failed())
    report_->error(1359, "{} is truncated or corrupt.", filename_);
  return lib_;
}

LibertyLibrary *
readLibDbFile(std::string_view filename,
              Network *network)
{
  // Validate .libdb + version, load string table, then LibLoader::read().
  Report *report = network->report();
  std::string path(filename);

  if (!filename.ends_with(".libdb"))
    report->error(1361, "{} must end with .libdb.", path);

  FILE *f = fopen(path.c_str(), "rb");
  if (f == nullptr)
    report->error(1366, "cannot open {}.", path);

  // File layout: [header][string_count][string bytes][body bytes]
  LibDbHeader hdr{};
  if (fread(&hdr, sizeof hdr, 1, f) != 1) {
    fclose(f);
    report->error(1355, "{} is truncated.", path);
  }
  if (hdr.version != lib_db_version) {
    fclose(f);
    report->error(1362, "{} is liberty database version {}, expected {}.",
                  path, hdr.version, lib_db_version);
  }

  uint32_t string_count = 0;
  bool ok = fread(&string_count, sizeof string_count, 1, f) == 1;
  std::vector<uint8_t> string_bytes(hdr.string_bytes);
  if (ok && hdr.string_bytes)
    ok = fread(string_bytes.data(), hdr.string_bytes, 1, f) == 1;
  std::vector<uint8_t> body(hdr.body_bytes);
  if (ok && hdr.body_bytes)
    ok = fread(body.data(), hdr.body_bytes, 1, f) == 1;
  fclose(f);
  if (!ok)
    report->error(1363, "{} is truncated.", path);

  // Unpack (length, characters)* into the string list DbReader::str() uses.
  std::vector<std::string> strings;
  strings.reserve(string_count);
  size_t pos = 0;
  for (uint32_t i = 0; i < string_count; i++) {
    uint32_t len = 0;
    bool len_ok = pos + sizeof len <= string_bytes.size();
    if (len_ok) {
      std::memcpy(&len, string_bytes.data() + pos, sizeof len);
      pos += sizeof len;
    }
    if (!len_ok || pos + len > string_bytes.size())
      report->error(1356, "{} has a corrupt string table.", path);
    strings.emplace_back(reinterpret_cast<const char *>(string_bytes.data() + pos), len);
    pos += len;
  }

  // Hand body + string list to the loader; it rebuilds the library object.
  DbReader reader(body.data(), body.size(), &strings);
  LibLoader loader(reader, filename, network);
  return loader.read();
}

} // namespace sta
