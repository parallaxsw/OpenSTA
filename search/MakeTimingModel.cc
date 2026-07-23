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

#include "MakeTimingModel.hh"
#include "MakeTimingModelPvt.hh"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ArcDelayCalc.hh"
#include "ClkDelays.hh"
#include "Clock.hh"
#include "ContainerHelpers.hh"
#include "Debug.hh"
#include "Delay.hh"
#include "Graph.hh"
#include "GraphClass.hh"
#include "GraphDelayCalc.hh"
#include "Liberty.hh"
#include "LibertyClass.hh"
#include "Network.hh"
#include "NetworkClass.hh"
#include "Path.hh"
#include "PathEnd.hh"
#include "PortDirection.hh"
#include "RiseFallMinMax.hh"
#include "Scene.hh"
#include "Sdc.hh"
#include "SdcClass.hh"
#include "Search.hh"
#include "Sta.hh"
#include "StaState.hh"
#include "TableModel.hh"
#include "TimingArc.hh"
#include "TimingRole.hh"
#include "Transition.hh"
#include "Units.hh"
#include "VisitPathEnds.hh"
#include "liberty/LibertyBuilder.hh"

namespace sta {

LibertyLibrary *
makeTimingModel(std::string_view lib_name,
                std::string_view cell_name,
                std::string_view filename,
                const Scene *scene,
                Sta *sta)
{
  MakeTimingModel maker(lib_name, cell_name, filename, scene, sta);
  return maker.makeTimingModel();
}

MakeTimingModel::MakeTimingModel(std::string_view lib_name,
                                 std::string_view cell_name,
                                 std::string_view filename,
                                 const Scene *scene,
                                 Sta *sta) :
  StaState(sta),
  lib_name_(lib_name),
  cell_name_(cell_name),
  filename_(filename),
  scene_(scene),
  min_max_(MinMax::max()),
  lib_builder_(new LibertyBuilder(debug_,
                                  report_)),
  sdc_(scene->sdc()),
  sta_(sta)
{
  scenes_.insert(scene_);
}

MakeTimingModel::~MakeTimingModel() { delete lib_builder_; }

LibertyLibrary *
MakeTimingModel::makeTimingModel()
{
  saveSdc();

  tbl_template_index_ = 1;
  makeLibrary();
  makeCell();
  makePorts();

  sta_->searchPreamble();

  findTimingFromInputs();
  findClkedOutputPaths();
  findClkTreeDelays();

  cell_->finish(false, report_, debug_);
  restoreSdc();

  return library_;
}

// Move sdc commands used by makeTimingModel to the side.
void
MakeTimingModel::saveSdc()
{
  sdc_backup_ = new Sdc(sdc_->mode(), this);
  swapSdcWithBackup();
  sta_->delaysInvalid();
}

void
MakeTimingModel::restoreSdc()
{
  swapSdcWithBackup();
  delete sdc_backup_;
  sta_->delaysInvalid();
}

void
MakeTimingModel::swapSdcWithBackup()
{
  Sdc::swapPortDelays(sdc_, sdc_backup_);
  Sdc::swapPortExtCaps(sdc_, sdc_backup_);
  Sdc::swapDeratingFactors(sdc_, sdc_backup_);
  Sdc::swapClockInsertions(sdc_, sdc_backup_);
}

void
MakeTimingModel::makeLibrary()
{
  library_ = network_->makeLibertyLibrary(lib_name_, filename_);
  LibertyLibrary *default_lib = network_->defaultLibertyLibrary();
  *library_->units() = *default_lib->units();

  for (const RiseFall *rf : RiseFall::range()) {
    library_->setInputThreshold(rf, default_lib->inputThreshold(rf));
    library_->setOutputThreshold(rf, default_lib->outputThreshold(rf));
    library_->setSlewLowerThreshold(rf, default_lib->slewLowerThreshold(rf));
    library_->setSlewUpperThreshold(rf, default_lib->slewUpperThreshold(rf));
  }

  library_->setDelayModelType(default_lib->delayModelType());
  library_->setNominalProcess(default_lib->nominalProcess());
  library_->setNominalVoltage(default_lib->nominalVoltage());
  library_->setNominalTemperature(default_lib->nominalTemperature());
}

void
MakeTimingModel::makeCell()
{
  cell_ = lib_builder_->makeCell(library_, cell_name_, filename_);
  cell_->setIsMacro(true);
  cell_->setArea(findArea());
}

float
MakeTimingModel::findArea()
{
  float area = 0.0;
  LeafInstanceIterator *leaf_iter = network_->leafInstanceIterator();
  while (leaf_iter->hasNext()) {
    const Instance *inst = leaf_iter->next();
    const LibertyCell *cell = network_->libertyCell(inst);
    if (cell)
      area += cell->area();
  }
  delete leaf_iter;
  return area;
}

void
MakeTimingModel::makePorts()
{
  Instance *top_inst = network_->topInstance();
  Cell *top_cell = network_->cell(top_inst);
  CellPortIterator *port_iter = network_->portIterator(top_cell);
  while (port_iter->hasNext()) {
    Port *port = port_iter->next();
    std::string port_name(network_->name(port));
    if (network_->isBus(port)) {
      int from_index = network_->fromIndex(port);
      int to_index = network_->toIndex(port);
      BusDcl *bus_dcl = library_->makeBusDcl(port_name, from_index, to_index);
      LibertyPort *lib_port =
          lib_builder_->makeBusPort(cell_, port_name, from_index, to_index, bus_dcl);
      lib_port->setDirection(network_->direction(port));
      PortMemberIterator *member_iter = network_->memberIterator(port);
      while (member_iter->hasNext()) {
        Port *bit_port = member_iter->next();
        Pin *pin = network_->findPin(top_inst, bit_port);
        LibertyPort *lib_bit_port = modelPort(pin);
        float load_cap = graph_delay_calc_->loadCap(pin, scene_, min_max_);
        lib_bit_port->setCapacitance(load_cap);
      }
      delete member_iter;
    }
    else {
      LibertyPort *lib_port = lib_builder_->makePort(cell_, port_name);
      lib_port->setDirection(network_->direction(port));
      Pin *pin = network_->findPin(top_inst, port);
      float load_cap = graph_delay_calc_->loadCap(pin, scene_, min_max_);
      lib_port->setCapacitance(load_cap);
    }
  }
  delete port_iter;
}

void
MakeTimingModel::checkClock(Clock *clk)
{
  for (const Pin *pin : clk->leafPins()) {
    if (!network_->isTopLevelPort(pin))
      report_->warn(1380, "clock {} pin {} is inside model block.", clk->name(),
                    network_->pathName(pin));
  }
}

////////////////////////////////////////////////////////////////

class MakeEndTimingArcs : public PathEndVisitor
{
public:
  MakeEndTimingArcs(const ModelClockSources &clk_sources,
                    std::map<const Pin*, ClockEdgeDelays> &margins,
                    Sta *sta);
  MakeEndTimingArcs(const MakeEndTimingArcs &) = default;
  PathEndVisitor *copy() const override;
  void visit(PathEnd *path_end) override;

private:
  const ModelClockSources &clk_sources_;
  std::map<const Pin*, ClockEdgeDelays> &margins_;
  Sta *sta_;
};

MakeEndTimingArcs::MakeEndTimingArcs(const ModelClockSources &clk_sources,
                                     std::map<const Pin*, ClockEdgeDelays> &margins,
                                     Sta *sta) :
  clk_sources_(clk_sources),
  margins_(margins),
  sta_(sta)
{
}

PathEndVisitor *
MakeEndTimingArcs::copy() const
{
  return new MakeEndTimingArcs(*this);
}

void
MakeEndTimingArcs::visit(PathEnd *path_end)
{
  Path *src_path = path_end->path();
  const Clock *src_clk = src_path->clock(sta_);
  const ClockEdge *tgt_clk_edge = path_end->targetClkEdge(sta_);
  if (src_clk && tgt_clk_edge) {
    const auto src_itr = clk_sources_.find(src_clk);
    if (src_itr == clk_sources_.end())
      return;
    const Pin *input_pin = src_itr->second.first;
    const RiseFall *input_rf = src_itr->second.second;
    Network *network = sta_->network();
    Debug *debug = sta_->debug();
    const MinMax *min_max = path_end->minMax(sta_);
    Arrival data_delay = src_path->arrival();
    Delay clk_latency = path_end->targetClkDelay(sta_);
    ArcDelay check_margin = path_end->margin(sta_);
    Delay margin = (min_max == MinMax::max())
      ? delaySum(delayDiff(data_delay, clk_latency, sta_), check_margin, sta_)
      : delaySum(delayDiff(clk_latency, data_delay, sta_), check_margin, sta_);
    float delay1 = delayAsFloat(margin, MinMax::max(), sta_);
    debugPrint(debug, "make_timing_model", 2, "{} -> {} clock {} {} {} {}",
               input_rf->shortName(), network->pathName(src_path->pin(sta_)),
               tgt_clk_edge->name(), path_end->typeName(),
               min_max->to_string(), delayAsString(margin, sta_));
    if (debug->check("make_timing_model", 3))
      sta_->reportPathEnd(path_end);

    RiseFallMinMax &margins = margins_[input_pin][tgt_clk_edge];
    float max_margin;
    bool max_exists;
    margins.value(input_rf, min_max, max_margin, max_exists);
    // Always max margin, even for min/hold checks.
    margins.setValue(input_rf, min_max,
                     max_exists ? std::max(max_margin, delay1) : delay1);
  }
}

// input -> register setup/hold
// input -> output combinational paths
// Batched: give each input in a batch its own virtual arrival clock so
// the arrival tags distinguish sources, then recover every input's
// register margins and output delays from one whole-graph propagation
// per batch instead of one filtered search per input port per
// rise/fall.
void
MakeTimingModel::findTimingFromInputs()
{
  search_->deleteFilteredArrivals();

  std::vector<const Pin*> input_pins;
  Instance *top_inst = network_->topInstance();
  Cell *top_cell = network_->cell(top_inst);
  CellPortBitIterator *port_iter = network_->portBitIterator(top_cell);
  while (port_iter->hasNext()) {
    Port *input_port = port_iter->next();
    if (network_->direction(input_port)->isInput()) {
      Pin *input_pin = network_->findPin(top_inst, input_port);
      if (input_pin && !sdc_->isClock(input_pin))
        input_pins.push_back(input_pin);
    }
  }
  delete port_iter;

  if (input_pins.empty())
    return;

  // Path count per vertex scales with the batch size, so the batch
  // size bounds search memory.
  constexpr size_t batch_size_max = 64;
  size_t batch_size = std::min(input_pins.size(), batch_size_max);
  std::vector<Clock*> clks(batch_size * RiseFall::index_count);
  for (size_t j = 0; j < batch_size; j++) {
    for (const RiseFall *rf : RiseFall::range())
      clks[j * RiseFall::index_count + rf->index()] = makeBatchClock(j, rf);
  }

  for (size_t begin = 0; begin < input_pins.size(); begin += batch_size) {
    size_t end = std::min(begin + batch_size, input_pins.size());
    std::vector<const Pin*> batch(input_pins.begin() + begin,
                                  input_pins.begin() + end);
    findTimingFromInputsBatch(batch, clks);
  }

  for (Clock *clk : clks)
    sta_->removeClock(clk, sdc_);

  // findClkedOutputPaths reads the clocked paths at the output pins;
  // iterate latch data arrivals to convergence so clock paths through
  // transparent latches reach the outputs.
  sta_->searchPreamble();
  search_->findAllArrivals();
}

Clock *
MakeTimingModel::makeBatchClock(size_t index,
                                const RiseFall *rf)
{
  std::string name = "make_timing_model_" + std::to_string(index)
    + "_" + rf->shortName();
  while (sdc_->findClock(name))
    name += "$";
  // Clone the default arrival clock waveform.
  FloatSeq waveform;
  waveform.push_back(0.0);
  waveform.push_back(0.0);
  sta_->makeClock(name, PinSet(network_), false, 0.0, waveform, "",
                  sdc_->mode());
  return sdc_->findClock(name);
}

void
MakeTimingModel::findTimingFromInputsBatch(const std::vector<const Pin*> &batch,
                                           const std::vector<Clock*> &clks)
{
  ModelClockSources clk_sources;
  for (size_t j = 0; j < batch.size(); j++) {
    const Pin *input_pin = batch[j];
    for (const RiseFall *input_rf : RiseFall::range()) {
      Clock *clk = clks[j * RiseFall::index_count + input_rf->index()];
      clk_sources[clk] = {input_pin, input_rf};
      sta_->setInputDelay(input_pin, input_rf->asRiseFallBoth(), clk,
                          RiseFall::rise(), nullptr, false, false,
                          MinMaxAll::all(), true, 0.0, sdc_);
    }
  }

  sta_->searchPreamble();
  search_->findFilteredArrivals(nullptr, nullptr, nullptr, false, false);

  std::map<const Pin*, ClockEdgeDelays> batch_margins;
  MakeEndTimingArcs end_visitor(clk_sources, batch_margins, sta_);
  VisitPathEnds visit_ends(sta_);
  for (Vertex *end : search_->endpoints())
    visit_ends.visitPathEnds(end, scenes_, MinMaxAll::all(), true, &end_visitor);

  std::map<const Pin*, OutputPinDelays> batch_output_delays;
  findOutputDelays(clk_sources, batch_output_delays);

  for (size_t j = 0; j < batch.size(); j++) {
    const Pin *input_pin = batch[j];
    for (const RiseFall *input_rf : RiseFall::range()) {
      Clock *clk = clks[j * RiseFall::index_count + input_rf->index()];
      sta_->removeInputDelay(input_pin, input_rf->asRiseFallBoth(), clk,
                             RiseFall::rise(), MinMaxAll::all(), sdc_);
    }
    makeSetupHoldTimingArcs(input_pin, batch_margins[input_pin]);
    makeInputOutputTimingArcs(input_pin, batch_output_delays[input_pin]);
  }
}

void
MakeTimingModel::findOutputDelays(const ModelClockSources &clk_sources,
                                  std::map<const Pin*, OutputPinDelays> &output_delays)
{
  InstancePinIterator *output_iter = network_->pinIterator(network_->topInstance());
  while (output_iter->hasNext()) {
    Pin *output_pin = output_iter->next();
    if (network_->direction(output_pin)->isOutput()) {
      Vertex *output_vertex = graph_->pinLoadVertex(output_pin);
      VertexPathIterator path_iter(output_vertex, this);
      while (path_iter.hasNext()) {
        Path *path = path_iter.next();
        const Clock *src_clk = path->clock(sta_);
        if (src_clk) {
          const auto src_itr = clk_sources.find(src_clk);
          if (src_itr != clk_sources.end()) {
            const Pin *input_pin = src_itr->second.first;
            const RiseFall *input_rf = src_itr->second.second;
            const RiseFall *output_rf = path->transition(sta_);
            const MinMax *min_max = path->minMax(sta_);
            Arrival delay = path->arrival();
            OutputDelays &delays = output_delays[input_pin][output_pin];
            delays.delays.mergeValue(output_rf, min_max,
                                     delayAsFloat(delay, min_max, sta_));
            delays.rf_path_exists[input_rf->index()][output_rf->index()] = true;
          }
        }
      }
    }
  }
  delete output_iter;
}

void
MakeTimingModel::makeSetupHoldTimingArcs(const Pin *input_pin,
                                         const ClockEdgeDelays &clk_margins)
{
  for (const auto &[clk_edge, margins] : clk_margins) {
    for (const MinMax *min_max : MinMax::range()) {
      bool setup = (min_max == MinMax::max());
      TimingArcAttrsPtr attrs = nullptr;
      for (const RiseFall *input_rf : RiseFall::range()) {
        float margin;
        bool exists;
        margins.value(input_rf, min_max, margin, exists);
        if (exists) {
          debugPrint(debug_, "make_timing_model", 2, "{} {} {} -> clock {} {}",
                     sta_->network()->pathName(input_pin), input_rf->shortName(),
                     min_max == MinMax::max() ? "setup" : "hold", clk_edge->name(),
                     delayAsString(margin, sta_));
          ScaleFactorType scale_type =
              setup ? ScaleFactorType::setup : ScaleFactorType::hold;
          TimingModel *check_model =
              makeScalarCheckModel(margin, scale_type, input_rf);
          if (attrs == nullptr)
            attrs = std::make_shared<TimingArcAttrs>();
          attrs->setModel(input_rf, check_model);
        }
      }
      if (attrs) {
        LibertyPort *input_port = modelPort(input_pin);
        for (const Pin *clk_pin : clk_edge->clock()->pins()) {
          LibertyPort *clk_port = modelPort(clk_pin);
          if (clk_port) {
            const RiseFall *clk_rf = clk_edge->transition();
            const TimingRole *role =
                setup ? TimingRole::setup() : TimingRole::hold();
            lib_builder_->makeFromTransitionArcs(cell_, clk_port, input_port,
                                                 nullptr, clk_rf, role, attrs);
          }
        }
      }
    }
  }
}

void
MakeTimingModel::makeInputOutputTimingArcs(const Pin *input_pin,
                                           OutputPinDelays &output_pin_delays)
{
  for (const auto &[output_pin, output_delays] : output_pin_delays) {
    TimingArcAttrsPtr attrs = nullptr;
    for (const RiseFall *output_rf : RiseFall::range()) {
      const MinMax *min_max = MinMax::max();
      float delay;
      bool exists;
      output_delays.delays.value(output_rf, min_max, delay, exists);
      if (exists) {
        debugPrint(debug_, "make_timing_model", 2, "{} -> {} {} delay {}",
                   network_->pathName(input_pin), network_->pathName(output_pin),
                   output_rf->shortName(), delayAsString(delay, sta_));
        TimingModel *gate_model = makeGateModelTable(output_pin, delay, output_rf);
        if (attrs == nullptr)
          attrs = std::make_shared<TimingArcAttrs>();
        attrs->setModel(output_rf, gate_model);
      }
    }
    if (attrs) {
      LibertyPort *output_port = modelPort(output_pin);
      LibertyPort *input_port = modelPort(input_pin);
      attrs->setTimingSense(output_delays.timingSense());
      lib_builder_->makeCombinationalArcs(cell_, input_port, output_port, true, true,
                                          attrs);
    }
  }
}

////////////////////////////////////////////////////////////////

// clocked register -> output paths
void
MakeTimingModel::findClkedOutputPaths()
{
  InstancePinIterator *output_iter = network_->pinIterator(network_->topInstance());
  while (output_iter->hasNext()) {
    Pin *output_pin = output_iter->next();
    if (network_->direction(output_pin)->isOutput()) {
      ClockEdgeDelays clk_delays;
      LibertyPort *output_port = modelPort(output_pin);
      Vertex *output_vertex = graph_->pinLoadVertex(output_pin);
      VertexPathIterator path_iter(output_vertex, this);
      while (path_iter.hasNext()) {
        Path *path = path_iter.next();
        const ClockEdge *clk_edge = path->clkEdge(sta_);
        if (clk_edge) {
          const RiseFall *output_rf = path->transition(sta_);
          const MinMax *min_max = path->minMax(sta_);
          Arrival delay = path->arrival();
          RiseFallMinMax &delays = clk_delays[clk_edge];
          delays.mergeValue(output_rf, min_max, delayAsFloat(delay, min_max, sta_));
        }
      }
      for (const auto &[clk_edge, delays] : clk_delays) {
        for (const Pin *clk_pin : clk_edge->clock()->pins()) {
          LibertyPort *clk_port = modelPort(clk_pin);
          if (clk_port) {
            const RiseFall *clk_rf = clk_edge->transition();
            TimingArcAttrsPtr attrs = nullptr;
            for (const RiseFall *output_rf : RiseFall::range()) {
              float delay = delays.value(output_rf, min_max_) - clk_edge->time();
              TimingModel *gate_model =
                  makeGateModelTable(output_pin, delay, output_rf);
              if (attrs == nullptr)
                attrs = std::make_shared<TimingArcAttrs>();
              attrs->setModel(output_rf, gate_model);
            }
            if (attrs) {
              lib_builder_->makeFromTransitionArcs(cell_, clk_port, output_port,
                                                   nullptr, clk_rf,
                                                   TimingRole::regClkToQ(), attrs);
            }
          }
        }
      }
    }
  }
  delete output_iter;
}

////////////////////////////////////////////////////////////////

void
MakeTimingModel::findClkTreeDelays()
{
  Instance *top_inst = network_->topInstance();
  Cell *top_cell = network_->cell(top_inst);
  CellPortIterator *port_iter = network_->portBitIterator(top_cell);
  while (port_iter->hasNext()) {
    Port *port = port_iter->next();
    if (network_->direction(port)->isInput()) {
      std::string port_name = network_->name(port);
      LibertyPort *lib_port = cell_->findLibertyPort(port_name);
      Pin *pin = network_->findPin(top_inst, port);
      if (pin && sdc_->isClock(pin)) {
        lib_port->setIsClock(true);
        ClockSet *clks = sdc_->findClocks(pin);
        if (clks->size() == 1) {
          for (const Clock *clk : *clks) {
            ClkDelays delays = sta_->findClkDelays(clk, scene_, true);
            for (const MinMax *min_max : MinMax::range()) {
              makeClkTreePaths(lib_port, min_max, TimingSense::positive_unate,
                               delays);
              makeClkTreePaths(lib_port, min_max, TimingSense::negative_unate,
                               delays);
            }
          }
        }
      }
    }
  }
  delete port_iter;
}

void
MakeTimingModel::makeClkTreePaths(LibertyPort *lib_port,
                                  const MinMax *min_max,
                                  TimingSense sense,
                                  const ClkDelays &delays)
{
  TimingArcAttrsPtr attrs = nullptr;
  for (const RiseFall *clk_rf : RiseFall::range()) {
    const RiseFall *end_rf =
        (sense == TimingSense::positive_unate) ? clk_rf : clk_rf->opposite();
    Path clk_path;
    Delay insertion, delay, latency;
    float lib_clk_delay;
    bool exists;
    delays.delay(clk_rf, end_rf, min_max, insertion, delay, lib_clk_delay, latency,
                 clk_path, exists);
    if (exists) {
      TimingModel *model = makeGateModelScalar(delay, end_rf);
      if (attrs == nullptr)
        attrs = std::make_shared<TimingArcAttrs>();
      attrs->setModel(end_rf, model);
    }
  }
  if (attrs) {
    attrs->setTimingSense(sense);
    const TimingRole *role = (min_max == MinMax::min())
        ? TimingRole::clockTreePathMin()
        : TimingRole::clockTreePathMax();
    lib_builder_->makeClockTreePathArcs(cell_, lib_port, role, attrs);
  }
}

////////////////////////////////////////////////////////////////

LibertyPort *
MakeTimingModel::modelPort(const Pin *pin)
{
  return cell_->findLibertyPort(network_->name(network_->port(pin)));
}

TimingModel *
MakeTimingModel::makeScalarCheckModel(float value,
                                      ScaleFactorType scale_factor_type,
                                      const RiseFall *rf)
{
  TablePtr table = std::make_shared<Table>(value);
  TableTemplate *tbl_template =
    library_->findTableTemplate("scalar", TableTemplateType::delay);
  TableModel *check_table = new TableModel(table, tbl_template, scale_factor_type, rf);
  TableModels *check_tables = new TableModels(check_table);
  CheckTableModel *check = new CheckTableModel(cell_, check_tables);
  return check;
}

TimingModel *
MakeTimingModel::makeGateModelScalar(Delay delay,
                                     Slew slew,
                                     const RiseFall *rf)
{
  TablePtr delay_table = std::make_shared<Table>(delayAsFloat(delay));
  TablePtr slew_table = std::make_shared<Table>(delayAsFloat(slew));
  TableTemplate *tbl_template =
    library_->findTableTemplate("scalar", TableTemplateType::delay);
  TableModel *delay_model = new TableModel(delay_table, tbl_template,
                                           ScaleFactorType::cell, rf);
  TableModels *delay_models = new TableModels(delay_model);
  TableModel *slew_model = new TableModel(slew_table, tbl_template,
                                          ScaleFactorType::cell, rf);
  TableModels *slew_models = new TableModels(slew_model);
  GateTableModel *gate_model = new GateTableModel(cell_, delay_models, slew_models,
                                                  nullptr, nullptr);
  return gate_model;
}

TimingModel *
MakeTimingModel::makeGateModelScalar(Delay delay,
                                     const RiseFall *rf)
{
  TablePtr delay_table = std::make_shared<Table>(delayAsFloat(delay));
  TableTemplate *tbl_template =
    library_->findTableTemplate("scalar", TableTemplateType::delay);
  TableModel *delay_model = new TableModel(delay_table, tbl_template,
                                           ScaleFactorType::cell, rf);
  TableModels *models = new TableModels(delay_model);
  GateTableModel *gate_model = new GateTableModel(cell_, models, nullptr,
                                                  nullptr, nullptr);
  return gate_model;
}

// Eval the driver pin model along its load capacitance
// axis and add the input to output 'delay' to the table values.
TimingModel *
MakeTimingModel::makeGateModelTable(const Pin *output_pin,
                                    Delay delay,
                                    const RiseFall *rf)
{
  const Pvt *pvt = sdc_->operatingConditions(min_max_);
  PinSet *drvrs = network_->drivers(network_->net(network_->term(output_pin)));
  DcalcAPIndex ap_index = scene_->dcalcAnalysisPtIndex(min_max_);
  const Pin *drvr_pin = *drvrs->begin();
  const LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
  if (drvr_port) {
    const LibertyCell *drvr_cell = drvr_port->libertyCell();
    for (TimingArcSet *arc_set : drvr_cell->timingArcSetsTo(drvr_port)) {
      for (TimingArc *drvr_arc : arc_set->arcs()) {
        // Use the first timing arc to simplify life.
        if (drvr_arc->toEdge()->asRiseFall() == rf) {
          const LibertyPort *gate_in_port = drvr_arc->from();
          const Instance *drvr_inst = network_->instance(drvr_pin);
          const Pin *gate_in_pin = network_->findPin(drvr_inst, gate_in_port);
          if (gate_in_pin) {
            Vertex *gate_in_vertex = graph_->pinLoadVertex(gate_in_pin);
            Slew in_slew = graph_->slew(
                gate_in_vertex, drvr_arc->fromEdge()->asRiseFall(), ap_index);
            float in_slew1 = delayAsFloat(in_slew);
            GateTableModel *drvr_gate_model =
                drvr_arc->gateTableModel(scene_, min_max_);
            if (drvr_gate_model) {
              float output_load_cap = graph_delay_calc_->loadCap(output_pin,
                                                                 scene_,
                                                                 min_max_);
              float drvr_self_delay, drvr_self_slew;
              drvr_gate_model->gateDelay(pvt, in_slew1, output_load_cap,
                                         drvr_self_delay, drvr_self_slew);

              const TableModel *drvr_table = drvr_gate_model->delayModels()->model();
              const TableTemplate *drvr_template = drvr_table->tblTemplate();
              const TableAxis *drvr_load_axis = loadCapacitanceAxis(drvr_table);
              if (drvr_load_axis) {
                const FloatSeq &drvr_axis_values = drvr_load_axis->values();
                FloatSeq *load_values = new FloatSeq;
                FloatSeq *slew_values = new FloatSeq;
                for (float load_cap : drvr_axis_values) {
                  // get slew from driver input pin
                  float gate_delay, gate_slew;
                  drvr_gate_model->gateDelay(pvt, in_slew1, load_cap,
                                             gate_delay, gate_slew);
                  // Remove the self delay driving the output pin net load cap.
                  load_values->push_back(delayAsFloat(delay)
                                         + gate_delay
                                         - drvr_self_delay);
                  slew_values->push_back(delayAsFloat(gate_slew));
                }

                FloatSeq axis_values = drvr_axis_values;
                TableAxisPtr load_axis = std::make_shared<TableAxis>(
                    TableAxisVariable::total_output_net_capacitance,
                    std::move(axis_values));

                TablePtr delay_table =
                    std::make_shared<Table>(load_values, load_axis);
                TablePtr slew_table =
                    std::make_shared<Table>(slew_values, load_axis);

                TableTemplate *model_template =
                    ensureTableTemplate(drvr_template, load_axis);
                TableModel *delay_model = new TableModel(delay_table, model_template,
                                                         ScaleFactorType::cell, rf);
                TableModels *delay_models = new TableModels(delay_model);
                TableModel *slew_model = new TableModel(slew_table, model_template,
                                                        ScaleFactorType::cell, rf);
                TableModels *slew_models = new TableModels(slew_model);
                GateTableModel *gate_model = new GateTableModel(cell_,
                                                                delay_models,
                                                                slew_models,
                                                                nullptr, nullptr);
                return gate_model;
              }
            }
          }
        }
      }
    }
  }
  Vertex *output_vertex = graph_->pinLoadVertex(output_pin);
  Slew slew = graph_->slew(output_vertex, rf, ap_index);
  return makeGateModelScalar(delay, slew, rf);
}

TableTemplate *
MakeTimingModel::ensureTableTemplate(const TableTemplate *drvr_template,
                                     const TableAxisPtr &load_axis)
{
  TableTemplate *model_template = findKey(template_map_, drvr_template);
  if (model_template == nullptr) {
    std::string template_name = "template_";
    template_name += std::to_string(tbl_template_index_++);

    model_template =
        library_->makeTableTemplate(template_name, TableTemplateType::delay);
    model_template->setAxis1(load_axis);
    template_map_[drvr_template] = model_template;
  }
  return model_template;
}

const TableAxis *
MakeTimingModel::loadCapacitanceAxis(const TableModel *table)
{
  if (table->axis1()
      && table->axis1()->variable()
          == TableAxisVariable::total_output_net_capacitance)
    return table->axis1();
  else if (table->axis2()
           && table->axis2()->variable()
               == TableAxisVariable::total_output_net_capacitance)
    return table->axis2();
  else if (table->axis3()
           && table->axis3()->variable()
               == TableAxisVariable::total_output_net_capacitance)
    return table->axis3();
  else
    return nullptr;
}

OutputDelays::OutputDelays()
{
  rf_path_exists[RiseFall::riseIndex()][RiseFall::riseIndex()] = false;
  rf_path_exists[RiseFall::riseIndex()][RiseFall::fallIndex()] = false;
  rf_path_exists[RiseFall::fallIndex()][RiseFall::riseIndex()] = false;
  rf_path_exists[RiseFall::fallIndex()][RiseFall::fallIndex()] = false;
}

TimingSense
OutputDelays::timingSense() const
{
  if (rf_path_exists[RiseFall::riseIndex()][RiseFall::riseIndex()]
      && rf_path_exists[RiseFall::fallIndex()][RiseFall::fallIndex()]
      && !rf_path_exists[RiseFall::riseIndex()][RiseFall::fallIndex()]
      && !rf_path_exists[RiseFall::fallIndex()][RiseFall::riseIndex()])
    return TimingSense::positive_unate;
  else if (rf_path_exists[RiseFall::riseIndex()][RiseFall::fallIndex()]
           && rf_path_exists[RiseFall::fallIndex()][RiseFall::riseIndex()]
           && !rf_path_exists[RiseFall::riseIndex()][RiseFall::riseIndex()]
           && !rf_path_exists[RiseFall::fallIndex()][RiseFall::fallIndex()])
    return TimingSense::negative_unate;
  else if (rf_path_exists[RiseFall::riseIndex()][RiseFall::riseIndex()]
           || rf_path_exists[RiseFall::riseIndex()][RiseFall::fallIndex()]
           || rf_path_exists[RiseFall::fallIndex()][RiseFall::riseIndex()]
           || rf_path_exists[RiseFall::fallIndex()][RiseFall::fallIndex()])
    return TimingSense::non_unate;
  else
    return TimingSense::none;
}

}  // namespace sta
