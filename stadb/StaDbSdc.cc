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

#include "StaDbSdc.hh"

#include <algorithm>
#include <string>
#include <vector>

#include "Clock.hh"
#include "ClockGatingCheck.hh"
#include "ClockGroups.hh"
#include "ClockInsertion.hh"
#include "ClockLatency.hh"
#include "DataCheck.hh"
#include "DeratingFactors.hh"
#include "DisabledPorts.hh"
#include "ExceptionPath.hh"
#include "Format.hh"
#include "InputDrive.hh"
#include "Liberty.hh"
#include "MinMax.hh"
#include "Network.hh"
#include "PortDelay.hh"
#include "PortExtCap.hh"
#include "Sdc.hh"
#include "Sta.hh"
#include "StaDbCodec.hh"
#include "StaDbSections.hh"
#include "Transition.hh"
#include "Wireload.hh"

namespace sta {

// The writer and reader below declare their per-kind methods in the same order
// and with matching names. A kind that exists on one side and not the other is
// then obvious on inspection, which is the cheapest guard available for a
// stream this wide.

////////////////////////////////////////////////////////////////
//
// Singleton encodings.
//
// RiseFall and MinMax are encoded by their index, which is part of the
// analysis model rather than an incidental enumeration order, so it is not at
// risk of silent renumbering upstream. The "all" variants get an explicit
// third value.

constexpr uint8_t db_rf_rise = 0;
constexpr uint8_t db_rf_fall = 1;
constexpr uint8_t db_rf_both = 2;

static uint8_t
dbEncodeRfBoth(const RiseFallBoth *rf)
{
  if (rf == RiseFallBoth::rise())
    return db_rf_rise;
  if (rf == RiseFallBoth::fall())
    return db_rf_fall;
  return db_rf_both;
}

static const RiseFallBoth *
dbDecodeRfBoth(uint8_t code)
{
  switch (code) {
  case db_rf_rise: return RiseFallBoth::rise();
  case db_rf_fall: return RiseFallBoth::fall();
  case db_rf_both: return RiseFallBoth::riseFall();
  default: throw DbCorrupt("stadb sdc rise/fall code out of range");
  }
}

static uint8_t
dbEncodeMinMaxAll(const MinMaxAll *min_max)
{
  if (min_max == MinMaxAll::min())
    return 0;
  if (min_max == MinMaxAll::max())
    return 1;
  return 2;
}

static const MinMaxAll *
dbDecodeMinMaxAll(uint8_t code)
{
  switch (code) {
  case 0: return MinMaxAll::min();
  case 1: return MinMaxAll::max();
  case 2: return MinMaxAll::all();
  default: throw DbCorrupt("stadb sdc min/max code out of range");
  }
}

////////////////////////////////////////////////////////////////

// Encodes the SDC constraints. Network and liberty objects are referenced by
// path name rather than by index into the network section: a name is
// meaningful on its own, so a constraint that no longer resolves fails loudly
// instead of silently binding to whatever object holds that index.
class DbSdcWriter
{
public:
  DbSdcWriter(DbWriter &writer, Sta *sta);
  void write();

private:
  void writeEnvironment();
  void writeClocks();
  void writeClockAttrs();
  void writeClockGroups();
  void writePortDelays();
  void writeExceptions();
  void writeDataChecks();
  void writeDisables();
  void writeConstants();
  void writeLoads();
  void writeDerating();
  void writeDesignRules();

  void writeException(ExceptionPath *exception);
  void writeExceptionPts(ExceptionPath *exception);
  void writeExceptionFrom(ExceptionFrom *from);
  void writeExceptionThru(ExceptionThru *thru);
  void writeExceptionTo(ExceptionTo *to);
  void writeDeratingFactors(const DeratingFactors *factors);
  void writeDeratingFactorsCell(const DeratingFactorsCell *factors);

  void kind(DbSdcKind kind) { writer_.putU8(static_cast<uint8_t>(kind)); }
  void putPin(const Pin *pin);
  void putNet(const Net *net);
  void putInstance(const Instance *inst);
  void putPort(const Port *port);
  void putCell(const Cell *cell);
  void putLibertyCell(const LibertyCell *cell);
  void putLibertyPort(const LibertyPort *port);
  void putClock(const Clock *clk);
  void putPinSet(const PinSet *pins);
  void putNetSet(const NetSet *nets);
  void putInstanceSet(const InstanceSet *insts);
  void putClockSet(const ClockSet *clks);
  void putRiseFallMinMax(const RiseFallMinMax &values);
  void putRiseFallValues(const RiseFallValues *values);
  void putMinMaxFloat(const MinMaxFloatValues &values);

  DbWriter &writer_;
  Sdc *sdc_;
  const Network *network_;
};

DbSdcWriter::DbSdcWriter(DbWriter &writer, Sta *sta) :
  writer_(writer),
  sdc_(sta->cmdSdc()),
  network_(sta->network())
{
}

////////////////////////////////////////////////////////////////

void
DbSdcWriter::putPin(const Pin *pin)
{
  writer_.putStr(pin ? network_->pathName(pin) : "");
}

void
DbSdcWriter::putNet(const Net *net)
{
  writer_.putStr(net ? network_->pathName(net) : "");
}

void
DbSdcWriter::putInstance(const Instance *inst)
{
  writer_.putStr(inst ? network_->pathName(inst) : "");
}

void
DbSdcWriter::putPort(const Port *port)
{
  writer_.putStr(port ? network_->name(port) : "");
}

void
DbSdcWriter::putCell(const Cell *cell)
{
  writer_.putStr(cell ? network_->name(network_->library(cell)) : "");
  writer_.putStr(cell ? network_->name(cell) : "");
}

void
DbSdcWriter::putLibertyCell(const LibertyCell *cell)
{
  writer_.putStr(cell ? cell->libertyLibrary()->name() : "");
  writer_.putStr(cell ? cell->name() : "");
}

void
DbSdcWriter::putLibertyPort(const LibertyPort *port)
{
  putLibertyCell(port ? port->libertyCell() : nullptr);
  writer_.putStr(port ? port->name() : "");
}

void
DbSdcWriter::putClock(const Clock *clk)
{
  writer_.putStr(clk ? clk->name() : "");
}

void
DbSdcWriter::putPinSet(const PinSet *pins)
{
  writer_.putU32(pins ? static_cast<uint32_t>(pins->size()) : 0);
  if (pins) {
    for (const Pin *pin : *pins)
      putPin(pin);
  }
}

void
DbSdcWriter::putNetSet(const NetSet *nets)
{
  writer_.putU32(nets ? static_cast<uint32_t>(nets->size()) : 0);
  if (nets) {
    for (const Net *net : *nets)
      putNet(net);
  }
}

void
DbSdcWriter::putInstanceSet(const InstanceSet *insts)
{
  writer_.putU32(insts ? static_cast<uint32_t>(insts->size()) : 0);
  if (insts) {
    for (const Instance *inst : *insts)
      putInstance(inst);
  }
}

void
DbSdcWriter::putClockSet(const ClockSet *clks)
{
  // Sorted by name because ClockSet is ordered by clock index, and a clock's
  // index depends on how many clocks preceded it, which the reader reproduces
  // only if every set it reads back is order independent.
  std::vector<const Clock*> sorted;
  if (clks)
    sorted.assign(clks->begin(), clks->end());
  std::sort(sorted.begin(), sorted.end(),
            [](const Clock *a, const Clock *b) { return a->name() < b->name(); });
  writer_.putU32(static_cast<uint32_t>(sorted.size()));
  for (const Clock *clk : sorted)
    putClock(clk);
}

void
DbSdcWriter::putRiseFallMinMax(const RiseFallMinMax &values)
{
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      values.value(rf, min_max, value, exists);
      writer_.putBool(exists);
      if (exists)
        writer_.putF32(value);
    }
  }
}

void
DbSdcWriter::putRiseFallValues(const RiseFallValues *values)
{
  for (const RiseFall *rf : RiseFall::range()) {
    float value;
    bool exists;
    if (values)
      values->value(rf, value, exists);
    else
      exists = false;
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(value);
  }
}

void
DbSdcWriter::putMinMaxFloat(const MinMaxFloatValues &values)
{
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    writer_.putBool(exists);
    if (exists)
      writer_.putF32(value);
  }
}

////////////////////////////////////////////////////////////////

void
DbSdcWriter::writeEnvironment()
{
  kind(DbSdcKind::analysis_type);
  writer_.putU8(static_cast<uint8_t>(sdc_->analysisType()));

  for (const MinMax *min_max : MinMax::range()) {
    OperatingConditions *op_cond = sdc_->operatingConditions(min_max);
    if (op_cond) {
      kind(DbSdcKind::operating_conditions);
      writer_.putU8(static_cast<uint8_t>(min_max->index()));
      writer_.putStr(op_cond->name());
    }
  }

  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    sdc_->voltage(min_max, value, exists);
    if (exists) {
      kind(DbSdcKind::voltage);
      writer_.putU8(static_cast<uint8_t>(min_max->index()));
      writer_.putF32(value);
    }
  }
  for (const auto &[net, values] : sdc_->net_voltage_map_) {
    kind(DbSdcKind::net_voltage);
    putNet(net);
    putMinMaxFloat(values);
  }

  for (const MinMax *min_max : MinMax::range()) {
    for (const auto &[inst, pvt] : sdc_->instance_pvt_maps_[min_max->index()]) {
      kind(DbSdcKind::instance_pvt);
      writer_.putU8(static_cast<uint8_t>(min_max->index()));
      putInstance(inst);
      writer_.putF32(pvt->process());
      writer_.putF32(pvt->voltage());
      writer_.putF32(pvt->temperature());
    }
  }

  for (const MinMax *min_max : MinMax::range()) {
    Wireload *wireload = sdc_->wireload(min_max);
    if (wireload) {
      kind(DbSdcKind::wireload);
      writer_.putU8(static_cast<uint8_t>(min_max->index()));
      writer_.putStr(wireload->name());
    }
    const WireloadSelection *selection = sdc_->wireloadSelection(min_max);
    if (selection) {
      kind(DbSdcKind::wireload_selection);
      writer_.putU8(static_cast<uint8_t>(min_max->index()));
      writer_.putStr(selection->name());
    }
  }
  kind(DbSdcKind::wireload_mode);
  writer_.putU8(static_cast<uint8_t>(sdc_->wireloadMode()));
}

void
DbSdcWriter::writeClocks()
{
  // Definition order, which fixes each clock's index and therefore the
  // iteration order of every clock-keyed container.
  for (Clock *clk : sdc_->clocks()) {
    if (clk->isGenerated()) {
      kind(DbSdcKind::generated_clock);
      writer_.putStr(clk->name());
      putPinSet(&clk->pins());
      writer_.putBool(clk->addToPins());
      putPin(clk->srcPin());
      putClock(clk->masterClkInfered() ? nullptr : clk->masterClk());
      writer_.putI32(clk->divideBy());
      writer_.putI32(clk->multiplyBy());
      writer_.putF32(clk->dutyCycle());
      writer_.putBool(clk->invert());
      writer_.putBool(clk->combinational());
      const IntSeq &edges = clk->edges();
      writer_.putU32(static_cast<uint32_t>(edges.size()));
      for (int edge : edges)
        writer_.putI32(edge);
      const FloatSeq &shifts = clk->edgeShifts();
      writer_.putU32(static_cast<uint32_t>(shifts.size()));
      for (float shift : shifts)
        writer_.putF32(shift);
      writer_.putStr(clk->comment());
    }
    else {
      kind(DbSdcKind::clock);
      writer_.putStr(clk->name());
      putPinSet(&clk->pins());
      writer_.putBool(clk->addToPins());
      writer_.putF32(clk->period());
      const FloatSeq &waveform = clk->waveform();
      writer_.putU32(static_cast<uint32_t>(waveform.size()));
      for (float edge : waveform)
        writer_.putF32(edge);
      writer_.putStr(clk->comment());
    }
  }
}

void
DbSdcWriter::writeClockAttrs()
{
  for (Clock *clk : sdc_->clocks()) {
    if (clk->slews().hasValue()) {
      kind(DbSdcKind::clock_slew);
      putClock(clk);
      putRiseFallMinMax(clk->slews());
    }
    for (const MinMax *setup_hold : MinMax::range()) {
      float value;
      bool exists;
      clk->uncertainty(setup_hold, value, exists);
      if (exists) {
        kind(DbSdcKind::clock_uncertainty);
        putClock(clk);
        writer_.putU8(static_cast<uint8_t>(setup_hold->index()));
        writer_.putF32(value);
      }
    }
    for (size_t i = 0; i < path_clk_or_data_count; i++) {
      PathClkOrData clk_data = static_cast<PathClkOrData>(i);
      for (const RiseFall *rf : RiseFall::range()) {
        for (const MinMax *min_max : MinMax::range()) {
          float slew;
          bool exists;
          clk->slewLimit(rf, clk_data, min_max, slew, exists);
          if (exists) {
            kind(DbSdcKind::clock_slew_limit);
            putClock(clk);
            writer_.putU8(static_cast<uint8_t>(i));
            writer_.putU8(static_cast<uint8_t>(rf->index()));
            writer_.putU8(static_cast<uint8_t>(min_max->index()));
            writer_.putF32(slew);
          }
        }
      }
    }
    if (clk->isPropagated()) {
      kind(DbSdcKind::propagated_clock_pin);
      putClock(clk);
      putPin(nullptr);
    }
  }
  for (const Pin *pin : sdc_->propagated_clk_pins_) {
    kind(DbSdcKind::propagated_clock_pin);
    putClock(nullptr);
    putPin(pin);
  }

  for (const auto &[pin, uncertainties] : sdc_->pin_clk_uncertainty_map_) {
    kind(DbSdcKind::pin_uncertainty);
    putPin(pin);
    putMinMaxFloat(*uncertainties);
  }
  for (InterClockUncertainty *uncertainty : sdc_->inter_clk_uncertainties_) {
    kind(DbSdcKind::inter_clock_uncertainty);
    putClock(uncertainty->src());
    putClock(uncertainty->target());
    for (const RiseFall *rf : RiseFall::range())
      putRiseFallMinMax(*uncertainty->uncertainties(rf));
  }

  for (ClockLatency *latency : *sdc_->clockLatencies()) {
    kind(DbSdcKind::clock_latency);
    putClock(latency->clock());
    putPin(latency->pin());
    putRiseFallMinMax(latency->delays());
  }
  for (ClockInsertion *insertion : sdc_->clockInsertions()) {
    kind(DbSdcKind::clock_insertion);
    putClock(insertion->clock());
    putPin(insertion->pin());
    for (const EarlyLate *early_late : EarlyLate::range())
      putRiseFallMinMax(insertion->delays(early_late));
  }

  if (sdc_->clk_gating_check_) {
    kind(DbSdcKind::clock_gating_check);
    writer_.putU8(0);
    putClock(nullptr);
    putRiseFallMinMax(sdc_->clk_gating_check_->margins());
    writer_.putU8(static_cast<uint8_t>(sdc_->clk_gating_check_->activeValue()));
  }
  for (const auto &[clk, check] : sdc_->clk_gating_check_map_) {
    kind(DbSdcKind::clock_gating_check);
    writer_.putU8(1);
    putClock(clk);
    putRiseFallMinMax(check->margins());
    writer_.putU8(static_cast<uint8_t>(check->activeValue()));
  }
  for (const auto &[inst, check] : sdc_->inst_clk_gating_check_map_) {
    kind(DbSdcKind::clock_gating_check);
    writer_.putU8(2);
    putInstance(inst);
    putRiseFallMinMax(check->margins());
    writer_.putU8(static_cast<uint8_t>(check->activeValue()));
  }
  for (const auto &[pin, check] : sdc_->pin_clk_gating_check_map_) {
    kind(DbSdcKind::clock_gating_check);
    writer_.putU8(3);
    putPin(pin);
    putRiseFallMinMax(check->margins());
    writer_.putU8(static_cast<uint8_t>(check->activeValue()));
  }

  for (const auto &[pin_clk, sense] : sdc_->clk_sense_map_) {
    kind(DbSdcKind::clock_sense);
    putPin(pin_clk.first);
    putClock(pin_clk.second);
    writer_.putU8(static_cast<uint8_t>(sense));
  }
}

void
DbSdcWriter::writeClockGroups()
{
  for (const auto &[name, groups] : sdc_->clk_groups_name_map_) {
    kind(DbSdcKind::clock_groups);
    writer_.putStr(name);
    writer_.putBool(groups->logicallyExclusive());
    writer_.putBool(groups->physicallyExclusive());
    writer_.putBool(groups->asynchronous());
    writer_.putBool(groups->allowPaths());
    writer_.putStr(groups->comment());
    writer_.putU32(static_cast<uint32_t>(groups->groups()->size()));
    for (ClockSet *clks : *groups->groups())
      putClockSet(clks);
  }
}

void
DbSdcWriter::writePortDelays()
{
  for (InputDelay *delay : sdc_->inputDelays()) {
    kind(DbSdcKind::input_delay);
    putPin(delay->pin());
    const ClockEdge *clk_edge = delay->clkEdge();
    putClock(clk_edge ? clk_edge->clock() : nullptr);
    writer_.putU8(clk_edge
                  ? static_cast<uint8_t>(clk_edge->transition()->index())
                  : 0);
    putPin(delay->refPin());
    writer_.putBool(delay->sourceLatencyIncluded());
    writer_.putBool(delay->networkLatencyIncluded());
    putRiseFallMinMax(delay->delays());
  }
  for (OutputDelay *delay : sdc_->outputDelays()) {
    kind(DbSdcKind::output_delay);
    putPin(delay->pin());
    const ClockEdge *clk_edge = delay->clkEdge();
    putClock(clk_edge ? clk_edge->clock() : nullptr);
    writer_.putU8(clk_edge
                  ? static_cast<uint8_t>(clk_edge->transition()->index())
                  : 0);
    putPin(delay->refPin());
    writer_.putBool(delay->sourceLatencyIncluded());
    writer_.putBool(delay->networkLatencyIncluded());
    putRiseFallMinMax(delay->delays());
  }
}

void
DbSdcWriter::writeExceptionFrom(ExceptionFrom *from)
{
  writer_.putBool(from != nullptr);
  if (from) {
    putPinSet(from->pins());
    putClockSet(from->clks());
    putInstanceSet(from->instances());
    writer_.putU8(dbEncodeRfBoth(from->transition()));
  }
}

void
DbSdcWriter::writeExceptionThru(ExceptionThru *thru)
{
  putPinSet(thru->pins());
  putNetSet(thru->nets());
  putInstanceSet(thru->instances());
  writer_.putU8(dbEncodeRfBoth(thru->transition()));
}

void
DbSdcWriter::writeExceptionTo(ExceptionTo *to)
{
  writer_.putBool(to != nullptr);
  if (to) {
    putPinSet(to->pins());
    putClockSet(to->clks());
    putInstanceSet(to->instances());
    writer_.putU8(dbEncodeRfBoth(to->transition()));
    writer_.putU8(dbEncodeRfBoth(to->endTransition()));
  }
}

void
DbSdcWriter::writeExceptionPts(ExceptionPath *exception)
{
  writeExceptionFrom(exception->from());
  ExceptionThruSeq *thrus = exception->thrus();
  writer_.putU32(thrus ? static_cast<uint32_t>(thrus->size()) : 0);
  if (thrus) {
    for (ExceptionThru *thru : *thrus)
      writeExceptionThru(thru);
  }
  writeExceptionTo(exception->to());
}

void
DbSdcWriter::writeException(ExceptionPath *exception)
{
  writer_.putU8(static_cast<uint8_t>(exception->type()));
  writer_.putU8(dbEncodeMinMaxAll(exception->minMax()));
  writer_.putStr(exception->comment());
  switch (exception->type()) {
  case ExceptionPathType::multi_cycle:
    writer_.putBool(exception->useEndClk());
    writer_.putI32(exception->pathMultiplier());
    break;
  case ExceptionPathType::path_delay:
    writer_.putBool(exception->ignoreClkLatency());
    writer_.putBool(exception->breakPath());
    writer_.putF32(exception->delay());
    break;
  case ExceptionPathType::path_margin:
    writer_.putF32(exception->margin());
    break;
  case ExceptionPathType::group_path:
    writer_.putStr(exception->name());
    writer_.putBool(exception->isDefault());
    break;
  default:
    break;
  }
  writeExceptionPts(exception);
}

void
DbSdcWriter::writeExceptions()
{
  // Named group paths are stored twice: a template in group_path_map_ and a
  // clone in exceptions_. Replaying the template recreates both, so the clone
  // is skipped here to avoid a duplicate on read.
  for (const auto &[name, groups] : sdc_->group_path_map_) {
    for (GroupPath *group_path : *groups) {
      kind(DbSdcKind::group_path);
      writeException(group_path);
    }
  }

  // exceptions_ is a set ordered by pointer address, which differs run to run,
  // so it is sorted before writing or the file would not be reproducible.
  std::vector<ExceptionPath*> sorted(sdc_->exceptions().begin(),
                                     sdc_->exceptions().end());
  sort(sorted, ExceptionPathLess(network_));
  for (ExceptionPath *exception : sorted) {
    // Loop exceptions are rebuilt from the graph and filters are transient.
    if (exception->type() != ExceptionPathType::loop
        && exception->type() != ExceptionPathType::filter
        && !(exception->type() == ExceptionPathType::group_path
             && !exception->name().empty())) {
      kind(DbSdcKind::exception);
      writeException(exception);
    }
  }
}

void
DbSdcWriter::writeDataChecks()
{
  for (const auto &[to_pin, checks] : sdc_->data_checks_to_map_) {
    for (DataCheck *check : *checks) {
      kind(DbSdcKind::data_check);
      putPin(check->from());
      putPin(check->to());
      putClock(check->clk());
      for (const RiseFall *from_rf : RiseFall::range()) {
        for (const RiseFall *to_rf : RiseFall::range()) {
          for (const MinMax *setup_hold : MinMax::range()) {
            float margin;
            bool exists;
            check->margin(from_rf, to_rf, setup_hold, margin, exists);
            writer_.putBool(exists);
            if (exists)
              writer_.putF32(margin);
          }
        }
      }
    }
  }
}

void
DbSdcWriter::writeDisables()
{
  for (const Pin *pin : sdc_->disabled_pins_) {
    kind(DbSdcKind::disable_pin);
    putPin(pin);
  }
  for (const Port *port : sdc_->disabled_ports_) {
    kind(DbSdcKind::disable_port);
    putPort(port);
  }
  for (const LibertyPort *port : sdc_->disabled_lib_ports_) {
    kind(DbSdcKind::disable_lib_port);
    putLibertyPort(port);
  }
  for (const auto &[cell, disabled] : sdc_->disabled_cell_ports_) {
    kind(DbSdcKind::disable_cell_ports);
    putLibertyCell(cell);
    writer_.putBool(disabled->all());
    LibertyPortPairSet *from_to = disabled->fromTo();
    writer_.putU32(from_to ? static_cast<uint32_t>(from_to->size()) : 0);
    if (from_to) {
      for (const auto &[from, to] : *from_to) {
        putLibertyPort(from);
        putLibertyPort(to);
      }
    }
    LibertyPortSet *from = disabled->from();
    writer_.putU32(from ? static_cast<uint32_t>(from->size()) : 0);
    if (from) {
      for (const LibertyPort *port : *from)
        putLibertyPort(port);
    }
    LibertyPortSet *to = disabled->to();
    writer_.putU32(to ? static_cast<uint32_t>(to->size()) : 0);
    if (to) {
      for (const LibertyPort *port : *to)
        putLibertyPort(port);
    }
  }
  for (const auto &[inst, disabled] : sdc_->disabled_inst_ports_) {
    kind(DbSdcKind::disable_inst_ports);
    putInstance(inst);
    writer_.putBool(disabled->all());
    LibertyPortPairSet *from_to = disabled->fromTo();
    writer_.putU32(from_to ? static_cast<uint32_t>(from_to->size()) : 0);
    if (from_to) {
      for (const auto &[from, to] : *from_to) {
        putLibertyPort(from);
        putLibertyPort(to);
      }
    }
    LibertyPortSet *from = disabled->from();
    writer_.putU32(from ? static_cast<uint32_t>(from->size()) : 0);
    if (from) {
      for (const LibertyPort *port : *from)
        putLibertyPort(port);
    }
    LibertyPortSet *to = disabled->to();
    writer_.putU32(to ? static_cast<uint32_t>(to->size()) : 0);
    if (to) {
      for (const LibertyPort *port : *to)
        putLibertyPort(port);
    }
  }
  for (const Instance *inst : *sdc_->disabledClockGatingChecksInst()) {
    kind(DbSdcKind::disable_gating_check);
    writer_.putU8(0);
    putInstance(inst);
  }
  for (const Pin *pin : *sdc_->disabledClockGatingChecksPin()) {
    kind(DbSdcKind::disable_gating_check);
    writer_.putU8(1);
    putPin(pin);
  }
  for (const LibertyCell *cell : *sdc_->disabledClockGatingChecksLibCell()) {
    kind(DbSdcKind::disable_gating_check);
    writer_.putU8(2);
    putLibertyCell(cell);
  }
}

void
DbSdcWriter::writeConstants()
{
  for (const auto &[pin, value] : sdc_->logicValues()) {
    kind(DbSdcKind::logic_value);
    putPin(pin);
    writer_.putU8(static_cast<uint8_t>(value));
  }
  for (const auto &[pin, value] : sdc_->caseLogicValues()) {
    kind(DbSdcKind::case_value);
    putPin(pin);
    writer_.putU8(static_cast<uint8_t>(value));
  }
}

void
DbSdcWriter::writeLoads()
{
  for (const auto &[port, ext_cap] : sdc_->port_ext_cap_map_) {
    kind(DbSdcKind::port_ext_cap);
    putPort(port);
    putRiseFallMinMax(ext_cap.pinCap());
    putRiseFallMinMax(ext_cap.wireCap());
    for (const MinMax *min_max : MinMax::range()) {
      int fanout;
      bool exists;
      ext_cap.fanout(min_max, fanout, exists);
      writer_.putBool(exists);
      if (exists)
        writer_.putI32(fanout);
    }
  }
  for (auto &[net, caps] : sdc_->net_wire_cap_map_) {
    kind(DbSdcKind::net_wire_cap);
    putNet(net);
    putMinMaxFloat(caps);
    for (const MinMax *min_max : MinMax::range())
      writer_.putBool(const_cast<NetWireCaps&>(caps).subtractPinCap(min_max));
  }
  for (const auto &[net, values] : sdc_->net_res_map_) {
    kind(DbSdcKind::net_resistance);
    putNet(net);
    putMinMaxFloat(values);
  }
  for (const auto &[port, drive] : sdc_->input_drive_map_) {
    kind(DbSdcKind::input_drive);
    putPort(port);
    putRiseFallMinMax(*drive->slews());
    // InputDrive exposes drive resistance one cell at a time rather than as a
    // block, so it is flattened here into the same layout as the slews.
    for (const RiseFall *rf : RiseFall::range()) {
      for (const MinMax *min_max : MinMax::range()) {
        float res;
        bool exists;
        drive->driveResistance(rf, min_max, res, exists);
        writer_.putBool(exists);
        if (exists)
          writer_.putF32(res);
      }
    }
    for (const RiseFall *rf : RiseFall::range()) {
      for (const MinMax *min_max : MinMax::range()) {
        const InputDriveCell *cell = drive->driveCell(rf, min_max);
        writer_.putBool(cell != nullptr);
        if (cell) {
          // The library is only set when -library was given on the command,
          // and it stays distinct from the cell's own library so that
          // write_sdc reproduces the original command.
          writer_.putStr(cell->library() ? cell->library()->name() : "");
          putLibertyCell(cell->cell());
          putLibertyPort(cell->fromPort());
          putLibertyPort(cell->toPort());
          const DriveCellSlews &slews = cell->fromSlews();
          writer_.putF32(slews[RiseFall::riseIndex()]);
          writer_.putF32(slews[RiseFall::fallIndex()]);
        }
      }
    }
  }
}

void
DbSdcWriter::writeDeratingFactors(const DeratingFactors *factors)
{
  for (size_t i = 0; i < path_clk_or_data_count; i++) {
    PathClkOrData clk_data = static_cast<PathClkOrData>(i);
    for (const RiseFall *rf : RiseFall::range()) {
      for (const EarlyLate *early_late : EarlyLate::range()) {
        float factor;
        bool exists;
        factors->factor(clk_data, rf, early_late, factor, exists);
        writer_.putBool(exists);
        if (exists)
          writer_.putF32(factor);
      }
    }
  }
}

void
DbSdcWriter::writeDeratingFactorsCell(const DeratingFactorsCell *factors)
{
  DeratingFactorsCell *cell = const_cast<DeratingFactorsCell*>(factors);
  for (size_t i = 0; i < timing_derate_cell_type_count; i++)
    writeDeratingFactors(cell->factors(static_cast<TimingDerateCellType>(i)));
}

void
DbSdcWriter::writeDerating()
{
  if (sdc_->derating_factors_) {
    kind(DbSdcKind::derating_global);
    for (size_t i = 0; i < timing_derate_type_count; i++) {
      TimingDerateType type = static_cast<TimingDerateType>(i);
      writeDeratingFactors(sdc_->derating_factors_->factors(type));
    }
  }
  for (const auto &[net, factors] : sdc_->net_derating_factors_) {
    kind(DbSdcKind::derating_net);
    putNet(net);
    writeDeratingFactors(factors);
  }
  for (const auto &[inst, factors] : sdc_->inst_derating_factors_) {
    kind(DbSdcKind::derating_inst);
    putInstance(inst);
    writeDeratingFactorsCell(factors);
  }
  for (const auto &[cell, factors] : sdc_->cell_derating_factors_) {
    kind(DbSdcKind::derating_cell);
    putLibertyCell(cell);
    writeDeratingFactorsCell(factors);
  }
}

void
DbSdcWriter::writeDesignRules()
{
  for (const auto &[port, values] : sdc_->port_slew_limit_map_) {
    kind(DbSdcKind::slew_limit_port);
    putPort(port);
    putMinMaxFloat(values);
  }
  for (const auto &[cell, values] : sdc_->cell_slew_limit_map_) {
    kind(DbSdcKind::slew_limit_cell);
    putCell(cell);
    putMinMaxFloat(values);
  }
  for (const auto &[port, values] : sdc_->port_cap_limit_map_) {
    kind(DbSdcKind::cap_limit_port);
    putPort(port);
    putMinMaxFloat(values);
  }
  for (const auto &[pin, values] : sdc_->pin_cap_limit_map_) {
    kind(DbSdcKind::cap_limit_pin);
    putPin(pin);
    putMinMaxFloat(values);
  }
  for (const auto &[cell, values] : sdc_->cell_cap_limit_map_) {
    kind(DbSdcKind::cap_limit_cell);
    putCell(cell);
    putMinMaxFloat(values);
  }
  for (const auto &[port, values] : sdc_->port_fanout_limit_map_) {
    kind(DbSdcKind::fanout_limit_port);
    putPort(port);
    putMinMaxFloat(values);
  }
  for (const auto &[cell, values] : sdc_->cell_fanout_limit_map_) {
    kind(DbSdcKind::fanout_limit_cell);
    putCell(cell);
    putMinMaxFloat(values);
  }

  for (const auto &[pin, limit] : sdc_->pin_latch_borrow_limit_map_) {
    kind(DbSdcKind::latch_borrow_limit);
    writer_.putU8(0);
    putPin(pin);
    writer_.putF32(limit);
  }
  for (const auto &[inst, limit] : sdc_->inst_latch_borrow_limit_map_) {
    kind(DbSdcKind::latch_borrow_limit);
    writer_.putU8(1);
    putInstance(inst);
    writer_.putF32(limit);
  }
  for (const auto &[clk, limit] : sdc_->clk_latch_borrow_limit_map_) {
    kind(DbSdcKind::latch_borrow_limit);
    writer_.putU8(2);
    putClock(clk);
    writer_.putF32(limit);
  }

  kind(DbSdcKind::min_pulse_width);
  writer_.putU8(0);
  putPin(nullptr);
  putRiseFallValues(&sdc_->min_pulse_width_);
  for (const auto &[pin, values] : sdc_->pin_min_pulse_width_map_) {
    kind(DbSdcKind::min_pulse_width);
    writer_.putU8(1);
    putPin(pin);
    putRiseFallValues(values);
  }
  for (const auto &[inst, values] : sdc_->inst_min_pulse_width_map_) {
    kind(DbSdcKind::min_pulse_width);
    writer_.putU8(2);
    putInstance(inst);
    putRiseFallValues(values);
  }
  for (const auto &[clk, values] : sdc_->clk_min_pulse_width_map_) {
    kind(DbSdcKind::min_pulse_width);
    writer_.putU8(3);
    putClock(clk);
    putRiseFallValues(values);
  }

  kind(DbSdcKind::max_area);
  writer_.putF32(sdc_->maxArea());
  kind(DbSdcKind::max_dynamic_power);
  writer_.putF32(sdc_->maxDynamicPower());
  kind(DbSdcKind::max_leakage_power);
  writer_.putF32(sdc_->maxLeakagePower());
}

void
DbSdcWriter::write()
{
  writeEnvironment();
  writeClocks();
  writeClockAttrs();
  writeClockGroups();
  writePortDelays();
  writeExceptions();
  writeDataChecks();
  writeDisables();
  writeConstants();
  writeLoads();
  writeDerating();
  writeDesignRules();
  kind(DbSdcKind::end);
}

////////////////////////////////////////////////////////////////

// Replays the constraint stream through the same Sdc entry points the Tcl
// commands use, so every derived index and cache is rebuilt as a side effect
// rather than being stored and restored.
class DbSdcReader
{
public:
  DbSdcReader(DbReader &reader, Sta *sta);
  void read();

private:
  void readAnalysisType();
  void readOperatingConditions();
  void readVoltage();
  void readNetVoltage();
  void readInstancePvt();
  void readWireload();
  void readWireloadMode();
  void readWireloadSelection();
  void readClock();
  void readGeneratedClock();
  void readClockSlew();
  void readClockUncertainty();
  void readClockSlewLimit();
  void readPropagatedClock();
  void readPinUncertainty();
  void readInterClockUncertainty();
  void readClockLatency();
  void readClockInsertion();
  void readClockGroups();
  void readClockSense();
  void readClockGatingCheck();
  void readPortDelay(bool is_input);
  void readException(bool is_group_path_template);
  void readDataCheck();
  void readDisablePin();
  void readDisablePort();
  void readDisableLibPort();
  void readDisableCellPorts();
  void readDisableInstPorts();
  void readDisableGatingCheck();
  void readLogicValue(bool is_case);
  void readPortExtCap();
  void readNetWireCap();
  void readNetResistance();
  void readInputDrive();
  void readDeratingGlobal();
  void readDeratingNet();
  void readDeratingInst();
  void readDeratingCell();
  void readSlewLimitPort();
  void readSlewLimitCell();
  void readCapLimitPort();
  void readCapLimitPin();
  void readCapLimitCell();
  void readFanoutLimitPort();
  void readFanoutLimitCell();
  void readLatchBorrowLimit();
  void readMinPulseWidth();

  ExceptionFrom *readExceptionFrom();
  ExceptionThruSeq *readExceptionThrus();
  ExceptionTo *readExceptionTo();
  void readDeratingFactors(DeratingFactors *factors);
  void readDeratingFactorsCell(DeratingFactorsCell *factors);

  const Pin *getPin();
  Pin *getPinEdit();
  const Net *getNet();
  const Instance *getInstance();
  Port *getPort();
  Cell *getCell();
  LibertyCell *getLibertyCell();
  LibertyPort *getLibertyPort();
  Clock *getClock();
  PinSet *getPinSet();
  NetSet *getNetSet();
  InstanceSet *getInstanceSet();
  ClockSet *getClockSet();
  RiseFallMinMax getRiseFallMinMax();
  void getRiseFallValues(RiseFallValues &values);
  MinMaxFloatValues getMinMaxFloat();
  const MinMax *getMinMax();
  const RiseFall *getRiseFall();

  DbReader &reader_;
  Sdc *sdc_;
  Network *network_;
};

DbSdcReader::DbSdcReader(DbReader &reader, Sta *sta) :
  reader_(reader),
  sdc_(sta->cmdSdc()),
  network_(sta->network())
{
}

////////////////////////////////////////////////////////////////

// The empty name that the getters map to null belongs to fields the sdc
// genuinely leaves unset. A set member is not one of those: the sets compare
// their members by reading an id off the object, so a null that reaches one
// dereferences null rather than failing the read. Hence dbCheck on every
// member read below.

const Pin *
DbSdcReader::getPin()
{
  std::string_view name = reader_.getStr();
  if (name.empty())
    return nullptr;
  const Pin *pin = network_->findPin(name);
  if (pin == nullptr)
    throw DbCorrupt(sta::format("stadb sdc pin {} not found", name));
  return pin;
}

Pin *
DbSdcReader::getPinEdit()
{
  return const_cast<Pin*>(getPin());
}

const Net *
DbSdcReader::getNet()
{
  std::string_view name = reader_.getStr();
  if (name.empty())
    return nullptr;
  const Net *net = network_->findNet(name);
  if (net == nullptr)
    throw DbCorrupt(sta::format("stadb sdc net {} not found", name));
  return net;
}

const Instance *
DbSdcReader::getInstance()
{
  std::string_view name = reader_.getStr();
  if (name.empty())
    return nullptr;
  const Instance *inst = network_->findInstance(name);
  if (inst == nullptr)
    throw DbCorrupt(sta::format("stadb sdc instance {} not found", name));
  return inst;
}

Port *
DbSdcReader::getPort()
{
  std::string_view name = reader_.getStr();
  if (name.empty())
    return nullptr;
  Cell *top = network_->cell(network_->topInstance());
  Port *port = network_->findPort(top, name);
  if (port == nullptr)
    throw DbCorrupt(sta::format("stadb sdc port {} not found", name));
  return port;
}

Cell *
DbSdcReader::getCell()
{
  std::string_view lib_name = reader_.getStr();
  std::string_view cell_name = reader_.getStr();
  if (lib_name.empty() && cell_name.empty())
    return nullptr;
  Library *library = network_->findLibrary(lib_name);
  Cell *cell = library ? network_->findCell(library, cell_name) : nullptr;
  if (cell == nullptr)
    throw DbCorrupt(sta::format("stadb sdc cell {}/{} not found", lib_name,
                                cell_name));
  return cell;
}

LibertyCell *
DbSdcReader::getLibertyCell()
{
  std::string_view lib_name = reader_.getStr();
  std::string_view cell_name = reader_.getStr();
  if (lib_name.empty() && cell_name.empty())
    return nullptr;
  LibertyLibrary *library = network_->findLibertyFilename(lib_name);
  if (library == nullptr)
    library = network_->findLiberty(lib_name);
  LibertyCell *cell = library ? library->findLibertyCell(cell_name) : nullptr;
  if (cell == nullptr)
    throw DbCorrupt(sta::format("stadb sdc liberty cell {}/{} not found",
                                lib_name, cell_name));
  return cell;
}

LibertyPort *
DbSdcReader::getLibertyPort()
{
  LibertyCell *cell = getLibertyCell();
  std::string_view port_name = reader_.getStr();
  if (cell == nullptr)
    return nullptr;
  LibertyPort *port = cell->findLibertyPort(port_name);
  if (port == nullptr)
    throw DbCorrupt(sta::format("stadb sdc liberty port {}/{} not found",
                                cell->name(), port_name));
  return port;
}

Clock *
DbSdcReader::getClock()
{
  std::string_view name = reader_.getStr();
  if (name.empty())
    return nullptr;
  Clock *clk = sdc_->findClock(name);
  if (clk == nullptr)
    throw DbCorrupt(sta::format("stadb sdc clock {} not found", name));
  return clk;
}

PinSet *
DbSdcReader::getPinSet()
{
  size_t count = reader_.getCount("sdc pin set");
  PinSet *pins = new PinSet(network_);
  for (size_t i = 0; i < count; i++)
    pins->insert(dbCheck(getPin(), "pin set member"));
  return pins;
}

NetSet *
DbSdcReader::getNetSet()
{
  size_t count = reader_.getCount("sdc net set");
  NetSet *nets = new NetSet(network_);
  for (size_t i = 0; i < count; i++)
    nets->insert(dbCheck(getNet(), "net set member"));
  return nets;
}

InstanceSet *
DbSdcReader::getInstanceSet()
{
  size_t count = reader_.getCount("sdc instance set");
  InstanceSet *insts = new InstanceSet(network_);
  for (size_t i = 0; i < count; i++)
    insts->insert(dbCheck(getInstance(), "instance set member"));
  return insts;
}

ClockSet *
DbSdcReader::getClockSet()
{
  size_t count = reader_.getCount("sdc clock set");
  ClockSet *clks = new ClockSet;
  for (size_t i = 0; i < count; i++)
    clks->insert(dbCheck(getClock(), "clock set member"));
  return clks;
}

RiseFallMinMax
DbSdcReader::getRiseFallMinMax()
{
  RiseFallMinMax values;
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      if (reader_.getBool())
        values.setValue(rf, min_max, reader_.getF32());
    }
  }
  return values;
}

void
DbSdcReader::getRiseFallValues(RiseFallValues &values)
{
  for (const RiseFall *rf : RiseFall::range()) {
    if (reader_.getBool())
      values.setValue(rf, reader_.getF32());
  }
}

MinMaxFloatValues
DbSdcReader::getMinMaxFloat()
{
  MinMaxFloatValues values;
  for (const MinMax *min_max : MinMax::range()) {
    if (reader_.getBool())
      values.setValue(min_max, reader_.getF32());
  }
  return values;
}

const MinMax *
DbSdcReader::getMinMax()
{
  uint8_t index = reader_.getU8();
  if (index >= MinMax::index_count)
    throw DbCorrupt("stadb sdc min/max index out of range");
  return MinMax::find(static_cast<int>(index));
}

const RiseFall *
DbSdcReader::getRiseFall()
{
  uint8_t index = reader_.getU8();
  if (index >= RiseFall::index_count)
    throw DbCorrupt("stadb sdc rise/fall index out of range");
  return RiseFall::find(static_cast<int>(index));
}

////////////////////////////////////////////////////////////////

void
DbSdcReader::readAnalysisType()
{
  uint8_t type = reader_.getU8();
  if (type > static_cast<uint8_t>(AnalysisType::ocv))
    throw DbCorrupt("stadb sdc analysis type out of range");
  sdc_->setAnalysisType(static_cast<AnalysisType>(type));
}

void
DbSdcReader::readOperatingConditions()
{
  const MinMax *min_max = getMinMax();
  std::string name(reader_.getStr());
  OperatingConditions *op_cond = nullptr;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext() && op_cond == nullptr)
    op_cond = lib_iter->next()->findOperatingConditions(name);
  delete lib_iter;
  if (op_cond == nullptr)
    throw DbCorrupt(sta::format("stadb sdc operating conditions {} not found",
                                name));
  sdc_->setOperatingConditions(op_cond, min_max);
}

void
DbSdcReader::readVoltage()
{
  const MinMax *min_max = getMinMax();
  sdc_->setVoltage(min_max, reader_.getF32());
}

void
DbSdcReader::readNetVoltage()
{
  const Net *net = getNet();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setVoltage(net, min_max, value);
  }
}

void
DbSdcReader::readInstancePvt()
{
  const MinMax *min_max = getMinMax();
  const Instance *inst = getInstance();
  Pvt pvt(reader_.getF32(), reader_.getF32(), reader_.getF32());
  sdc_->setPvt(inst, min_max->asMinMaxAll(), pvt);
}

void
DbSdcReader::readWireload()
{
  const MinMax *min_max = getMinMax();
  std::string name(reader_.getStr());
  Wireload *wireload = nullptr;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext() && wireload == nullptr)
    wireload = const_cast<Wireload*>(lib_iter->next()->findWireload(name));
  delete lib_iter;
  if (wireload == nullptr)
    throw DbCorrupt(sta::format("stadb sdc wireload {} not found", name));
  sdc_->setWireload(wireload, min_max->asMinMaxAll());
}

void
DbSdcReader::readWireloadSelection()
{
  const MinMax *min_max = getMinMax();
  std::string name(reader_.getStr());
  const WireloadSelection *selection = nullptr;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext() && selection == nullptr)
    selection = lib_iter->next()->findWireloadSelection(name);
  delete lib_iter;
  if (selection == nullptr)
    throw DbCorrupt(sta::format("stadb sdc wireload selection {} not found",
                                name));
  sdc_->setWireloadSelection(selection, min_max->asMinMaxAll());
}

void
DbSdcReader::readWireloadMode()
{
  uint8_t mode = reader_.getU8();
  sdc_->setWireloadMode(static_cast<WireloadMode>(mode));
}

void
DbSdcReader::readClock()
{
  std::string name(reader_.getStr());
  PinSet *pins = getPinSet();
  bool add_to_pins = reader_.getBool();
  float period = reader_.getF32();
  uint32_t edge_count = reader_.getU32();
  FloatSeq waveform;
  for (uint32_t i = 0; i < edge_count; i++)
    waveform.push_back(reader_.getF32());
  std::string comment(reader_.getStr());
  sdc_->makeClock(name, *pins, add_to_pins, period, waveform, comment);
  delete pins;
}

void
DbSdcReader::readGeneratedClock()
{
  std::string name(reader_.getStr());
  PinSet *pins = getPinSet();
  bool add_to_pins = reader_.getBool();
  Pin *src_pin = getPinEdit();
  Clock *master_clk = getClock();
  int divide_by = reader_.getI32();
  int multiply_by = reader_.getI32();
  float duty_cycle = reader_.getF32();
  bool invert = reader_.getBool();
  bool combinational = reader_.getBool();
  IntSeq edges;
  uint32_t edge_count = reader_.getU32();
  for (uint32_t i = 0; i < edge_count; i++)
    edges.push_back(reader_.getI32());
  FloatSeq shifts;
  uint32_t shift_count = reader_.getU32();
  for (uint32_t i = 0; i < shift_count; i++)
    shifts.push_back(reader_.getF32());
  std::string comment(reader_.getStr());
  sdc_->makeGeneratedClock(name, *pins, add_to_pins, src_pin, master_clk,
                           divide_by, multiply_by, duty_cycle, invert,
                           combinational, edges, shifts, comment);
  delete pins;
}

void
DbSdcReader::readClockSlew()
{
  Clock *clk = dbCheck(getClock(), "clock slew clock");
  RiseFallMinMax slews = getRiseFallMinMax();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      slews.value(rf, min_max, value, exists);
      if (exists)
        clk->setSlew(rf, min_max, value);
    }
  }
}

void
DbSdcReader::readClockUncertainty()
{
  Clock *clk = dbCheck(getClock(), "clock uncertainty clock");
  const MinMax *setup_hold = getMinMax();
  clk->setUncertainty(setup_hold, reader_.getF32());
}

void
DbSdcReader::readClockSlewLimit()
{
  Clock *clk = dbCheck(getClock(), "clock slew limit clock");
  uint8_t clk_data = reader_.getU8();
  if (clk_data >= path_clk_or_data_count)
    throw DbCorrupt("stadb sdc clock slew limit type out of range");
  const RiseFall *rf = getRiseFall();
  const MinMax *min_max = getMinMax();
  clk->setSlewLimit(rf->asRiseFallBoth(), static_cast<PathClkOrData>(clk_data),
                    min_max, reader_.getF32());
}

void
DbSdcReader::readPropagatedClock()
{
  Clock *clk = getClock();
  Pin *pin = getPinEdit();
  if (clk)
    sdc_->setPropagatedClock(clk);
  else
    sdc_->setPropagatedClock(pin);
}

void
DbSdcReader::readPinUncertainty()
{
  Pin *pin = getPinEdit();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *setup_hold : MinMax::range()) {
    float value;
    bool exists;
    values.value(setup_hold, value, exists);
    if (exists)
      sdc_->setClockUncertainty(pin, setup_hold->asMinMaxAll(), value);
  }
}

void
DbSdcReader::readInterClockUncertainty()
{
  Clock *from_clk = dbCheck(getClock(), "inter clock uncertainty source");
  Clock *to_clk = dbCheck(getClock(), "inter clock uncertainty target");
  for (const RiseFall *from_rf : RiseFall::range()) {
    RiseFallMinMax values = getRiseFallMinMax();
    for (const RiseFall *to_rf : RiseFall::range()) {
      for (const MinMax *setup_hold : MinMax::range()) {
        float value;
        bool exists;
        values.value(to_rf, setup_hold, value, exists);
        if (exists)
          sdc_->setClockUncertainty(from_clk, from_rf->asRiseFallBoth(), to_clk,
                                    to_rf->asRiseFallBoth(),
                                    setup_hold->asMinMaxAll(), value);
      }
    }
  }
}

void
DbSdcReader::readClockLatency()
{
  Clock *clk = getClock();
  Pin *pin = getPinEdit();
  RiseFallMinMax delays = getRiseFallMinMax();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      delays.value(rf, min_max, value, exists);
      if (exists)
        sdc_->setClockLatency(clk, pin, rf->asRiseFallBoth(),
                              min_max->asMinMaxAll(), value);
    }
  }
}

void
DbSdcReader::readClockInsertion()
{
  Clock *clk = getClock();
  Pin *pin = getPinEdit();
  for (const EarlyLate *early_late : EarlyLate::range()) {
    RiseFallMinMax delays = getRiseFallMinMax();
    for (const RiseFall *rf : RiseFall::range()) {
      for (const MinMax *min_max : MinMax::range()) {
        float value;
        bool exists;
        delays.value(rf, min_max, value, exists);
        if (exists)
          sdc_->setClockInsertion(clk, pin, rf, min_max, early_late, value);
      }
    }
  }
}

void
DbSdcReader::readClockGroups()
{
  std::string name(reader_.getStr());
  bool logically_exclusive = reader_.getBool();
  bool physically_exclusive = reader_.getBool();
  bool asynchronous = reader_.getBool();
  bool allow_paths = reader_.getBool();
  std::string comment(reader_.getStr());
  ClockGroups *groups = sdc_->makeClockGroups(name, logically_exclusive,
                                              physically_exclusive,
                                              asynchronous, allow_paths,
                                              comment);
  uint32_t group_count = reader_.getU32();
  for (uint32_t i = 0; i < group_count; i++)
    sdc_->makeClockGroup(groups, getClockSet());
}

void
DbSdcReader::readClockSense()
{
  // Stored one pin/clock pair at a time, since that is how the map holds it.
  PinSet *pins = new PinSet(network_);
  pins->insert(dbCheck(getPin(), "clock sense pin"));
  ClockSet *clks = new ClockSet;
  clks->insert(dbCheck(getClock(), "clock sense clock"));
  uint8_t sense = reader_.getU8();
  if (sense > static_cast<uint8_t>(ClockSense::stop))
    throw DbCorrupt("stadb sdc clock sense out of range");
  sdc_->setClockSense(pins, clks, static_cast<ClockSense>(sense));
}

void
DbSdcReader::readClockGatingCheck()
{
  uint8_t scope = reader_.getU8();
  Clock *clk = nullptr;
  const Instance *inst = nullptr;
  const Pin *pin = nullptr;
  switch (scope) {
  case 0: reader_.getStr(); break;
  case 1: clk = dbCheck(getClock(), "clock gating check clock"); break;
  case 2: inst = dbCheck(getInstance(), "clock gating check instance"); break;
  case 3: pin = dbCheck(getPin(), "clock gating check pin"); break;
  default: throw DbCorrupt("stadb sdc clock gating scope out of range");
  }
  RiseFallMinMax margins = getRiseFallMinMax();
  uint8_t active_value = reader_.getU8();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *setup_hold : MinMax::range()) {
      float value;
      bool exists;
      margins.value(rf, setup_hold, value, exists);
      if (exists) {
        switch (scope) {
        case 0:
          sdc_->setClockGatingCheck(rf->asRiseFallBoth(), setup_hold, value);
          break;
        case 1:
          sdc_->setClockGatingCheck(clk, rf->asRiseFallBoth(), setup_hold,
                                    value);
          break;
        case 2:
          sdc_->setClockGatingCheck(const_cast<Instance*>(inst),
                                    rf->asRiseFallBoth(), setup_hold, value,
                                    static_cast<LogicValue>(active_value));
          break;
        case 3:
          sdc_->setClockGatingCheck(pin, rf->asRiseFallBoth(), setup_hold,
                                    value,
                                    static_cast<LogicValue>(active_value));
          break;
        }
      }
    }
  }
}

void
DbSdcReader::readPortDelay(bool is_input)
{
  const Pin *pin = getPin();
  Clock *clk = getClock();
  const RiseFall *clk_rf = getRiseFall();
  const Pin *ref_pin = getPin();
  bool source_latency_included = reader_.getBool();
  bool network_latency_included = reader_.getBool();
  RiseFallMinMax delays = getRiseFallMinMax();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      delays.value(rf, min_max, value, exists);
      if (exists) {
        if (is_input)
          sdc_->setInputDelay(pin, rf->asRiseFallBoth(), clk, clk_rf, ref_pin,
                              source_latency_included, network_latency_included,
                              min_max->asMinMaxAll(), true, value);
        else
          sdc_->setOutputDelay(pin, rf->asRiseFallBoth(), clk, clk_rf, ref_pin,
                               source_latency_included,
                               network_latency_included,
                               min_max->asMinMaxAll(), true, value);
      }
    }
  }
}

ExceptionFrom *
DbSdcReader::readExceptionFrom()
{
  if (!reader_.getBool())
    return nullptr;
  PinSet *pins = getPinSet();
  ClockSet *clks = getClockSet();
  InstanceSet *insts = getInstanceSet();
  const RiseFallBoth *rf = dbDecodeRfBoth(reader_.getU8());
  return sdc_->makeExceptionFrom(pins, clks, insts, rf);
}

ExceptionThruSeq *
DbSdcReader::readExceptionThrus()
{
  uint32_t count = reader_.getU32();
  if (count == 0)
    return nullptr;
  ExceptionThruSeq *thrus = new ExceptionThruSeq;
  for (uint32_t i = 0; i < count; i++) {
    PinSet *pins = getPinSet();
    NetSet *nets = getNetSet();
    InstanceSet *insts = getInstanceSet();
    const RiseFallBoth *rf = dbDecodeRfBoth(reader_.getU8());
    thrus->push_back(sdc_->makeExceptionThru(pins, nets, insts, rf));
  }
  return thrus;
}

ExceptionTo *
DbSdcReader::readExceptionTo()
{
  if (!reader_.getBool())
    return nullptr;
  PinSet *pins = getPinSet();
  ClockSet *clks = getClockSet();
  InstanceSet *insts = getInstanceSet();
  const RiseFallBoth *rf = dbDecodeRfBoth(reader_.getU8());
  const RiseFallBoth *end_rf = dbDecodeRfBoth(reader_.getU8());
  return sdc_->makeExceptionTo(pins, clks, insts, rf, end_rf);
}

void
DbSdcReader::readException(bool)
{
  uint8_t type = reader_.getU8();
  const MinMaxAll *min_max = dbDecodeMinMaxAll(reader_.getU8());
  std::string comment(reader_.getStr());
  bool use_end_clk = false;
  int path_multiplier = 0;
  bool ignore_clk_latency = false;
  bool break_path = false;
  float delay = 0.0;
  float margin = 0.0;
  std::string group_name;
  bool is_default = false;
  switch (static_cast<ExceptionPathType>(type)) {
  case ExceptionPathType::multi_cycle:
    use_end_clk = reader_.getBool();
    path_multiplier = reader_.getI32();
    break;
  case ExceptionPathType::path_delay:
    ignore_clk_latency = reader_.getBool();
    break_path = reader_.getBool();
    delay = reader_.getF32();
    break;
  case ExceptionPathType::path_margin:
    margin = reader_.getF32();
    break;
  case ExceptionPathType::group_path:
    group_name = reader_.getStr();
    is_default = reader_.getBool();
    break;
  default:
    break;
  }
  ExceptionFrom *from = readExceptionFrom();
  ExceptionThruSeq *thrus = readExceptionThrus();
  ExceptionTo *to = readExceptionTo();

  switch (static_cast<ExceptionPathType>(type)) {
  case ExceptionPathType::false_path:
    sdc_->makeFalsePath(from, thrus, to, min_max, comment);
    break;
  case ExceptionPathType::multi_cycle:
    sdc_->makeMulticyclePath(from, thrus, to, min_max, use_end_clk,
                             path_multiplier, comment);
    break;
  case ExceptionPathType::path_delay:
    sdc_->makePathDelay(from, thrus, to, min_max->asMinMax(),
                        ignore_clk_latency, break_path, delay, comment);
    break;
  case ExceptionPathType::path_margin:
    sdc_->makePathMargin(from, thrus, to, min_max, margin, comment);
    break;
  case ExceptionPathType::group_path:
    sdc_->makeGroupPath(group_name, is_default, from, thrus, to, comment);
    break;
  default:
    throw DbCorrupt("stadb sdc exception type out of range");
  }
}

void
DbSdcReader::readDataCheck()
{
  Pin *from = getPinEdit();
  Pin *to = getPinEdit();
  Clock *clk = getClock();
  for (const RiseFall *from_rf : RiseFall::range()) {
    for (const RiseFall *to_rf : RiseFall::range()) {
      for (const MinMax *setup_hold : MinMax::range()) {
        if (reader_.getBool())
          sdc_->setDataCheck(from, from_rf->asRiseFallBoth(), to,
                             to_rf->asRiseFallBoth(), clk,
                             setup_hold->asMinMaxAll(), reader_.getF32());
      }
    }
  }
}

void
DbSdcReader::readDisablePin()
{
  sdc_->disable(getPin());
}

void
DbSdcReader::readDisablePort()
{
  sdc_->disable(getPort());
}

void
DbSdcReader::readDisableLibPort()
{
  sdc_->disable(getLibertyPort());
}

void
DbSdcReader::readDisableCellPorts()
{
  LibertyCell *cell = getLibertyCell();
  if (reader_.getBool())
    sdc_->disable(cell, nullptr, nullptr);
  uint32_t from_to_count = reader_.getU32();
  for (uint32_t i = 0; i < from_to_count; i++) {
    LibertyPort *from = getLibertyPort();
    sdc_->disable(cell, from, getLibertyPort());
  }
  uint32_t from_count = reader_.getU32();
  for (uint32_t i = 0; i < from_count; i++)
    sdc_->disable(cell, getLibertyPort(), nullptr);
  uint32_t to_count = reader_.getU32();
  for (uint32_t i = 0; i < to_count; i++)
    sdc_->disable(cell, nullptr, getLibertyPort());
}

void
DbSdcReader::readDisableInstPorts()
{
  Instance *inst = const_cast<Instance*>(getInstance());
  if (reader_.getBool())
    sdc_->disable(inst, nullptr, nullptr);
  uint32_t from_to_count = reader_.getU32();
  for (uint32_t i = 0; i < from_to_count; i++) {
    LibertyPort *from = getLibertyPort();
    sdc_->disable(inst, from, getLibertyPort());
  }
  uint32_t from_count = reader_.getU32();
  for (uint32_t i = 0; i < from_count; i++)
    sdc_->disable(inst, getLibertyPort(), nullptr);
  uint32_t to_count = reader_.getU32();
  for (uint32_t i = 0; i < to_count; i++)
    sdc_->disable(inst, nullptr, getLibertyPort());
}

void
DbSdcReader::readDisableGatingCheck()
{
  uint8_t scope = reader_.getU8();
  switch (scope) {
  case 0:
    sdc_->disableClockGatingCheck(const_cast<Instance*>(getInstance()));
    break;
  case 1:
    sdc_->disableClockGatingCheck(const_cast<Pin*>(getPin()));
    break;
  case 2:
    sdc_->disableClockGatingCheck(getLibertyCell());
    break;
  default:
    throw DbCorrupt("stadb sdc clock gating disable scope out of range");
  }
}

void
DbSdcReader::readLogicValue(bool is_case)
{
  const Pin *pin = getPin();
  uint8_t value = reader_.getU8();
  if (value > static_cast<uint8_t>(LogicValue::fall))
    throw DbCorrupt("stadb sdc logic value out of range");
  if (is_case)
    sdc_->setCaseAnalysis(pin, static_cast<LogicValue>(value));
  else
    sdc_->setLogicValue(pin, static_cast<LogicValue>(value));
}

void
DbSdcReader::readPortExtCap()
{
  const Port *port = getPort();
  RiseFallMinMax pin_cap = getRiseFallMinMax();
  RiseFallMinMax wire_cap = getRiseFallMinMax();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      pin_cap.value(rf, min_max, value, exists);
      if (exists)
        sdc_->setPortExtPinCap(port, rf, min_max, value);
      wire_cap.value(rf, min_max, value, exists);
      if (exists)
        sdc_->setPortExtWireCap(port, rf, min_max, value);
    }
  }
  for (const MinMax *min_max : MinMax::range()) {
    if (reader_.getBool())
      sdc_->setPortExtFanout(port, min_max, reader_.getI32());
  }
}

void
DbSdcReader::readNetWireCap()
{
  const Net *net = getNet();
  MinMaxFloatValues caps = getMinMaxFloat();
  bool subtract[MinMax::index_count];
  for (const MinMax *min_max : MinMax::range())
    subtract[min_max->index()] = reader_.getBool();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    caps.value(min_max, value, exists);
    if (exists)
      sdc_->setNetWireCap(net, subtract[min_max->index()], min_max, value);
  }
}

void
DbSdcReader::readNetResistance()
{
  const Net *net = getNet();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setResistance(net, min_max->asMinMaxAll(), value);
  }
}

void
DbSdcReader::readInputDrive()
{
  const Port *port = getPort();
  RiseFallMinMax slews = getRiseFallMinMax();
  RiseFallMinMax resistances = getRiseFallMinMax();
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      float value;
      bool exists;
      slews.value(rf, min_max, value, exists);
      if (exists)
        sdc_->setInputSlew(port, rf->asRiseFallBoth(), min_max->asMinMaxAll(),
                           value);
      resistances.value(rf, min_max, value, exists);
      if (exists)
        sdc_->setDriveResistance(port, rf->asRiseFallBoth(),
                                 min_max->asMinMaxAll(), value);
    }
  }
  for (const RiseFall *rf : RiseFall::range()) {
    for (const MinMax *min_max : MinMax::range()) {
      if (reader_.getBool()) {
        std::string lib_name(reader_.getStr());
        const LibertyLibrary *library = lib_name.empty()
          ? nullptr
          : network_->findLiberty(lib_name);
        LibertyCell *cell = getLibertyCell();
        LibertyPort *from_port = getLibertyPort();
        LibertyPort *to_port = getLibertyPort();
        DriveCellSlews from_slews;
        from_slews[RiseFall::riseIndex()] = reader_.getF32();
        from_slews[RiseFall::fallIndex()] = reader_.getF32();
        sdc_->setDriveCell(library, cell, port, from_port, from_slews, to_port,
                           rf->asRiseFallBoth(), min_max->asMinMaxAll());
      }
    }
  }
}

void
DbSdcReader::readDeratingFactors(DeratingFactors *factors)
{
  for (size_t i = 0; i < path_clk_or_data_count; i++) {
    PathClkOrData clk_data = static_cast<PathClkOrData>(i);
    for (const RiseFall *rf : RiseFall::range()) {
      for (const EarlyLate *early_late : EarlyLate::range()) {
        if (reader_.getBool())
          factors->setFactor(clk_data, rf->asRiseFallBoth(), early_late,
                             reader_.getF32());
      }
    }
  }
}

void
DbSdcReader::readDeratingFactorsCell(DeratingFactorsCell *factors)
{
  for (size_t i = 0; i < timing_derate_cell_type_count; i++)
    readDeratingFactors(factors->factors(static_cast<TimingDerateCellType>(i)));
}

void
DbSdcReader::readDeratingGlobal()
{
  for (size_t i = 0; i < timing_derate_type_count; i++) {
    TimingDerateType type = static_cast<TimingDerateType>(i);
    for (size_t j = 0; j < path_clk_or_data_count; j++) {
      PathClkOrData clk_data = static_cast<PathClkOrData>(j);
      RiseFallMinMax values = getRiseFallMinMax();
      for (const RiseFall *rf : RiseFall::range()) {
        for (const MinMax *early_late : MinMax::range()) {
          float value;
          bool exists;
          values.value(rf, early_late, value, exists);
          if (exists)
            sdc_->setTimingDerate(type, clk_data, rf->asRiseFallBoth(),
                                  early_late, value);
        }
      }
    }
  }
}

void
DbSdcReader::readDeratingNet()
{
  const Net *net = getNet();
  for (size_t i = 0; i < path_clk_or_data_count; i++) {
    PathClkOrData clk_data = static_cast<PathClkOrData>(i);
    RiseFallMinMax values = getRiseFallMinMax();
    for (const RiseFall *rf : RiseFall::range()) {
      for (const MinMax *early_late : MinMax::range()) {
        float value;
        bool exists;
        values.value(rf, early_late, value, exists);
        if (exists)
          sdc_->setTimingDerate(net, clk_data, rf->asRiseFallBoth(), early_late,
                                value);
      }
    }
  }
}

void
DbSdcReader::readDeratingInst()
{
  const Instance *inst = getInstance();
  for (size_t i = 0; i < timing_derate_cell_type_count; i++) {
    TimingDerateCellType type = static_cast<TimingDerateCellType>(i);
    for (size_t j = 0; j < path_clk_or_data_count; j++) {
      PathClkOrData clk_data = static_cast<PathClkOrData>(j);
      RiseFallMinMax values = getRiseFallMinMax();
      for (const RiseFall *rf : RiseFall::range()) {
        for (const MinMax *early_late : MinMax::range()) {
          float value;
          bool exists;
          values.value(rf, early_late, value, exists);
          if (exists)
            sdc_->setTimingDerate(inst, type, clk_data, rf->asRiseFallBoth(),
                                  early_late, value);
        }
      }
    }
  }
}

void
DbSdcReader::readDeratingCell()
{
  const LibertyCell *cell = getLibertyCell();
  for (size_t i = 0; i < timing_derate_cell_type_count; i++) {
    TimingDerateCellType type = static_cast<TimingDerateCellType>(i);
    for (size_t j = 0; j < path_clk_or_data_count; j++) {
      PathClkOrData clk_data = static_cast<PathClkOrData>(j);
      RiseFallMinMax values = getRiseFallMinMax();
      for (const RiseFall *rf : RiseFall::range()) {
        for (const MinMax *early_late : MinMax::range()) {
          float value;
          bool exists;
          values.value(rf, early_late, value, exists);
          if (exists)
            sdc_->setTimingDerate(cell, type, clk_data, rf->asRiseFallBoth(),
                                  early_late, value);
        }
      }
    }
  }
}

void
DbSdcReader::readSlewLimitPort()
{
  Port *port = getPort();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setSlewLimit(port, min_max, value);
  }
}

void
DbSdcReader::readSlewLimitCell()
{
  Cell *cell = getCell();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setSlewLimit(cell, min_max, value);
  }
}

void
DbSdcReader::readCapLimitPort()
{
  Port *port = getPort();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setCapacitanceLimit(port, min_max, value);
  }
}

void
DbSdcReader::readCapLimitPin()
{
  Pin *pin = getPinEdit();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setCapacitanceLimit(pin, min_max, value);
  }
}

void
DbSdcReader::readCapLimitCell()
{
  Cell *cell = getCell();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setCapacitanceLimit(cell, min_max, value);
  }
}

void
DbSdcReader::readFanoutLimitPort()
{
  Port *port = getPort();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setFanoutLimit(port, min_max, value);
  }
}

void
DbSdcReader::readFanoutLimitCell()
{
  Cell *cell = getCell();
  MinMaxFloatValues values = getMinMaxFloat();
  for (const MinMax *min_max : MinMax::range()) {
    float value;
    bool exists;
    values.value(min_max, value, exists);
    if (exists)
      sdc_->setFanoutLimit(cell, min_max, value);
  }
}

void
DbSdcReader::readLatchBorrowLimit()
{
  uint8_t scope = reader_.getU8();
  switch (scope) {
  case 0: {
    const Pin *pin = dbCheck(getPin(), "latch borrow limit pin");
    sdc_->setLatchBorrowLimit(pin, reader_.getF32());
    break;
  }
  case 1: {
    const Instance *inst = dbCheck(getInstance(), "latch borrow limit instance");
    sdc_->setLatchBorrowLimit(inst, reader_.getF32());
    break;
  }
  case 2: {
    const Clock *clk = dbCheck(getClock(), "latch borrow limit clock");
    sdc_->setLatchBorrowLimit(clk, reader_.getF32());
    break;
  }
  default:
    throw DbCorrupt("stadb sdc latch borrow scope out of range");
  }
}

void
DbSdcReader::readMinPulseWidth()
{
  uint8_t scope = reader_.getU8();
  const Pin *pin = nullptr;
  const Instance *inst = nullptr;
  const Clock *clk = nullptr;
  switch (scope) {
  case 0: reader_.getStr(); break;
  case 1: pin = dbCheck(getPin(), "min pulse width pin"); break;
  case 2: inst = dbCheck(getInstance(), "min pulse width instance"); break;
  case 3: clk = dbCheck(getClock(), "min pulse width clock"); break;
  default: throw DbCorrupt("stadb sdc min pulse width scope out of range");
  }
  RiseFallValues values;
  getRiseFallValues(values);
  for (const RiseFall *rf : RiseFall::range()) {
    float value;
    bool exists;
    values.value(rf, value, exists);
    if (exists) {
      switch (scope) {
      case 0: sdc_->setMinPulseWidth(rf->asRiseFallBoth(), value); break;
      case 1: sdc_->setMinPulseWidth(pin, rf->asRiseFallBoth(), value); break;
      case 2: sdc_->setMinPulseWidth(inst, rf->asRiseFallBoth(), value); break;
      case 3: sdc_->setMinPulseWidth(clk, rf->asRiseFallBoth(), value); break;
      }
    }
  }
}

void
DbSdcReader::read()
{
  for (;;) {
    uint8_t tag = reader_.getU8();
    switch (static_cast<DbSdcKind>(tag)) {
    case DbSdcKind::end: return;
    case DbSdcKind::analysis_type: readAnalysisType(); break;
    case DbSdcKind::operating_conditions: readOperatingConditions(); break;
    case DbSdcKind::voltage: readVoltage(); break;
    case DbSdcKind::net_voltage: readNetVoltage(); break;
    case DbSdcKind::instance_pvt: readInstancePvt(); break;
    case DbSdcKind::wireload: readWireload(); break;
    case DbSdcKind::wireload_mode: readWireloadMode(); break;
    case DbSdcKind::wireload_selection: readWireloadSelection(); break;
    case DbSdcKind::clock: readClock(); break;
    case DbSdcKind::generated_clock: readGeneratedClock(); break;
    case DbSdcKind::clock_slew: readClockSlew(); break;
    case DbSdcKind::clock_uncertainty: readClockUncertainty(); break;
    case DbSdcKind::clock_slew_limit: readClockSlewLimit(); break;
    case DbSdcKind::propagated_clock_pin: readPropagatedClock(); break;
    case DbSdcKind::pin_uncertainty: readPinUncertainty(); break;
    case DbSdcKind::inter_clock_uncertainty: readInterClockUncertainty(); break;
    case DbSdcKind::clock_latency: readClockLatency(); break;
    case DbSdcKind::clock_insertion: readClockInsertion(); break;
    case DbSdcKind::clock_groups: readClockGroups(); break;
    case DbSdcKind::clock_sense: readClockSense(); break;
    case DbSdcKind::clock_gating_check: readClockGatingCheck(); break;
    case DbSdcKind::input_delay: readPortDelay(true); break;
    case DbSdcKind::output_delay: readPortDelay(false); break;
    case DbSdcKind::exception: readException(false); break;
    case DbSdcKind::group_path: readException(true); break;
    case DbSdcKind::data_check: readDataCheck(); break;
    case DbSdcKind::disable_pin: readDisablePin(); break;
    case DbSdcKind::disable_port: readDisablePort(); break;
    case DbSdcKind::disable_lib_port: readDisableLibPort(); break;
    case DbSdcKind::disable_cell_ports: readDisableCellPorts(); break;
    case DbSdcKind::disable_inst_ports: readDisableInstPorts(); break;
    case DbSdcKind::disable_gating_check: readDisableGatingCheck(); break;
    case DbSdcKind::logic_value: readLogicValue(false); break;
    case DbSdcKind::case_value: readLogicValue(true); break;
    case DbSdcKind::port_ext_cap: readPortExtCap(); break;
    case DbSdcKind::net_wire_cap: readNetWireCap(); break;
    case DbSdcKind::net_resistance: readNetResistance(); break;
    case DbSdcKind::input_drive: readInputDrive(); break;
    case DbSdcKind::derating_global: readDeratingGlobal(); break;
    case DbSdcKind::derating_net: readDeratingNet(); break;
    case DbSdcKind::derating_inst: readDeratingInst(); break;
    case DbSdcKind::derating_cell: readDeratingCell(); break;
    case DbSdcKind::slew_limit_port: readSlewLimitPort(); break;
    case DbSdcKind::slew_limit_cell: readSlewLimitCell(); break;
    case DbSdcKind::cap_limit_port: readCapLimitPort(); break;
    case DbSdcKind::cap_limit_pin: readCapLimitPin(); break;
    case DbSdcKind::cap_limit_cell: readCapLimitCell(); break;
    case DbSdcKind::fanout_limit_port: readFanoutLimitPort(); break;
    case DbSdcKind::fanout_limit_cell: readFanoutLimitCell(); break;
    case DbSdcKind::latch_borrow_limit: readLatchBorrowLimit(); break;
    case DbSdcKind::min_pulse_width: readMinPulseWidth(); break;
    case DbSdcKind::max_area: sdc_->setMaxArea(reader_.getF32()); break;
    case DbSdcKind::max_dynamic_power:
      sdc_->setMaxDynamicPower(reader_.getF32());
      break;
    case DbSdcKind::max_leakage_power:
      sdc_->setMaxLeakagePower(reader_.getF32());
      break;
    default:
      throw DbCorrupt(sta::format("stadb sdc record kind {} unknown", tag));
    }
  }
}

////////////////////////////////////////////////////////////////

void
writeStaDbSdc(DbWriter &writer, Sta *sta)
{
  DbSdcWriter sdc_writer(writer, sta);
  sdc_writer.write();
}

void
readStaDbSdc(DbReader &reader, Sta *sta)
{
  DbSdcReader sdc_reader(reader, sta);
  sdc_reader.read();
}

} // namespace sta
