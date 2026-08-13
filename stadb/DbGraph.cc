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

#include "DbGraph.hh"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "DbCodec.hh"
#include "DbSections.hh"
#include "Format.hh"
#include "Graph.hh"
#include "GraphDelayCalc.hh"
#include "Liberty.hh"
#include "Network.hh"
#include "Sta.hh"
#include "TimingArc.hh"
#include "Variables.hh"
#include "search/Levelize.hh"

namespace sta {

// Vertex flag bits. Packed into one byte rather than written as separate
// booleans because there is one of these per pin, and a large design has
// millions of them.
constexpr uint8_t db_vertex_bidirect_drvr = 1 << 0;
constexpr uint8_t db_vertex_reg_clk = 1 << 1;
constexpr uint8_t db_vertex_has_checks = 1 << 2;
constexpr uint8_t db_vertex_is_check_clk = 1 << 3;
constexpr uint8_t db_vertex_downstream_clk_pin = 1 << 4;
constexpr uint8_t db_vertex_has_sim_value = 1 << 5;

constexpr uint8_t db_edge_bidirect_inst_path = 1 << 0;
constexpr uint8_t db_edge_bidirect_net_path = 1 << 1;
constexpr uint8_t db_edge_bidirect_port_path = 1 << 2;
constexpr uint8_t db_edge_disabled_loop = 1 << 3;
constexpr uint8_t db_edge_sim_sense = 1 << 4;
constexpr uint8_t db_edge_disabled_cond = 1 << 5;
constexpr uint8_t db_edge_incremental_annotation = 1 << 6;

////////////////////////////////////////////////////////////////

class DbGraphWriter
{
public:
  DbGraphWriter(DbWriter &writer, Sta *sta);
  void write();

private:
  void collect();
  void writeVertices();
  void writeEdges();
  void writeLevels();
  void writeDelayState();
  void writePeriodChecks();
  void putArcSet(const TimingArcSet *arc_set);
  uint32_t arcSetIndex(const TimingArcSet *arc_set);
  uint32_t vertexIndex(const Vertex *vertex) const;
  uint32_t edgeIndex(const Edge *edge) const;

  DbWriter &writer_;
  Sta *sta_;
  Graph *graph_;
  const Network *network_;
  DcalcAPIndex ap_count_;
  std::vector<Vertex*> vertices_;
  std::vector<Edge*> edges_;
  std::unordered_map<const Vertex*, uint32_t> vertex_indexes_;
  std::unordered_map<const Edge*, uint32_t> edge_indexes_;
  std::unordered_map<const TimingArcSet*, uint32_t> arc_set_indexes_;
};

DbGraphWriter::DbGraphWriter(DbWriter &writer, Sta *sta) :
  writer_(writer),
  sta_(sta),
  graph_(sta->graph()),
  network_(sta->network()),
  ap_count_(sta->dcalcAnalysisPtCount())
{
}

void
DbGraphWriter::collect()
{
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext())
    vertices_.push_back(vertex_iter.next());
  // Ascending id is creation order, which the reader replays to land every
  // object back on the id that the rest of the file refers to.
  std::sort(vertices_.begin(), vertices_.end(),
            [this](const Vertex *a, const Vertex *b) {
              return graph_->id(a) < graph_->id(b);
            });
  for (uint32_t i = 0; i < vertices_.size(); i++)
    vertex_indexes_[vertices_[i]] = i;

  for (Vertex *vertex : vertices_) {
    VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext())
      edges_.push_back(edge_iter.next());
  }
  std::sort(edges_.begin(), edges_.end(),
            [this](const Edge *a, const Edge *b) {
              return graph_->id(a) < graph_->id(b);
            });
  for (uint32_t i = 0; i < edges_.size(); i++)
    edge_indexes_[edges_[i]] = i;
}

uint32_t
DbGraphWriter::vertexIndex(const Vertex *vertex) const
{
  auto itr = vertex_indexes_.find(vertex);
  if (itr == vertex_indexes_.end())
    throw DbCorrupt("stadb graph edge references an unknown vertex");
  return itr->second;
}

uint32_t
DbGraphWriter::edgeIndex(const Edge *edge) const
{
  auto itr = edge_indexes_.find(edge);
  if (itr == edge_indexes_.end())
    throw DbCorrupt("stadb graph references an unknown edge");
  return itr->second;
}

uint32_t
DbGraphWriter::arcSetIndex(const TimingArcSet *arc_set)
{
  auto itr = arc_set_indexes_.find(arc_set);
  if (itr == arc_set_indexes_.end()) {
    // Index the whole cell at once. Arc sets are shared by every instance of
    // the cell, so the scan pays for itself immediately.
    const LibertyCell *cell = arc_set->libertyCell();
    uint32_t index = 0;
    for (const TimingArcSet *set : cell->timingArcSets())
      arc_set_indexes_[set] = index++;
    itr = arc_set_indexes_.find(arc_set);
    if (itr == arc_set_indexes_.end())
      throw DbCorrupt("stadb graph timing arc set is not owned by its cell");
  }
  return itr->second;
}

void
DbGraphWriter::putArcSet(const TimingArcSet *arc_set)
{
  // Wire arc sets are a process wide singleton with no cell to name them.
  bool is_wire = arc_set->isWire();
  writer_.putBool(is_wire);
  if (!is_wire) {
    const LibertyCell *cell = arc_set->libertyCell();
    writer_.putStr(cell->libertyLibrary()->name());
    writer_.putStr(cell->name());
    writer_.putU32(arcSetIndex(arc_set));
  }
}

void
DbGraphWriter::writeVertices()
{
  size_t slew_count = RiseFall::index_count * ap_count_;
  writer_.putU32(static_cast<uint32_t>(vertices_.size()));
  for (Vertex *vertex : vertices_) {
    writer_.putStr(network_->pathName(vertex->pin()));
    uint8_t flags = 0;
    if (vertex->isBidirectDriver())
      flags |= db_vertex_bidirect_drvr;
    if (vertex->isRegClk())
      flags |= db_vertex_reg_clk;
    if (vertex->hasChecks())
      flags |= db_vertex_has_checks;
    if (vertex->isCheckClk())
      flags |= db_vertex_is_check_clk;
    if (vertex->hasDownstreamClkPin())
      flags |= db_vertex_downstream_clk_pin;
    if (vertex->hasSimValue())
      flags |= db_vertex_has_sim_value;
    writer_.putU8(flags);
    writer_.putI32(vertex->level());
    for (const MinMax *min_max : MinMax::range()) {
      for (const RiseFall *rf : RiseFall::range())
        writer_.putBool(vertex->slewAnnotated(rf, min_max));
    }
    for (size_t i = 0; i < slew_count; i++)
      writer_.putF32(delayAsFloat(graph_->slew(vertex, i)));
  }
}

void
DbGraphWriter::writeEdges()
{
  writer_.putU32(static_cast<uint32_t>(edges_.size()));
  for (Edge *edge : edges_) {
    writer_.putU32(vertexIndex(edge->from(graph_)));
    writer_.putU32(vertexIndex(edge->to(graph_)));
    TimingArcSet *arc_set = edge->timingArcSet();
    putArcSet(arc_set);
    uint8_t flags = 0;
    if (edge->isBidirectInstPath())
      flags |= db_edge_bidirect_inst_path;
    if (edge->isBidirectNetPath())
      flags |= db_edge_bidirect_net_path;
    if (edge->isBidirectPortPath())
      flags |= db_edge_bidirect_port_path;
    if (edge->isDisabledLoop())
      flags |= db_edge_disabled_loop;
    if (edge->hasSimSense())
      flags |= db_edge_sim_sense;
    if (edge->hasDisabledCond())
      flags |= db_edge_disabled_cond;
    if (edge->delay_Annotation_Is_Incremental())
      flags |= db_edge_incremental_annotation;
    writer_.putU8(flags);
    // The annotated bit travels with each delay. Delay calc only writes into a
    // slot whose bit is clear, so losing it would either let a warm run
    // overwrite SDF values with liberty derived ones, or freeze computed
    // delays so that editing the netlist no longer updates them.
    for (const TimingArc *arc : arc_set->arcs()) {
      for (DcalcAPIndex ap = 0; ap < ap_count_; ap++) {
        writer_.putF32(delayAsFloat(graph_->arcDelay(edge, arc, ap)));
        writer_.putBool(graph_->arcDelayAnnotated(edge, arc, ap));
      }
    }
  }
}

void
DbGraphWriter::writeLevels()
{
  Levelize *levelize = sta_->levelize();
  writer_.putBool(levelize->levelized());
  writer_.putI32(levelize->maxLevel());
  VertexSet &roots = levelize->roots();
  std::vector<uint32_t> root_indexes;
  for (Vertex *root : roots)
    root_indexes.push_back(vertexIndex(root));
  std::sort(root_indexes.begin(), root_indexes.end());
  writer_.putU32(static_cast<uint32_t>(root_indexes.size()));
  for (uint32_t index : root_indexes)
    writer_.putU32(index);

  // Loops matter beyond levelization: Sdc::makeLoopExceptions turns each one
  // into a false path, so dropping them would change reported timing on any
  // design with a combinational loop.
  GraphLoopSeq &loops = levelize->loops();
  writer_.putU32(static_cast<uint32_t>(loops.size()));
  for (GraphLoop *loop : loops) {
    EdgeSeq *loop_edges = loop->edges();
    writer_.putU32(static_cast<uint32_t>(loop_edges->size()));
    for (Edge *edge : *loop_edges)
      writer_.putU32(edgeIndex(edge));
  }
}

// Sorted so that the file does not inherit the hash order of a VertexSet,
// which varies with allocation addresses and would break byte idempotence.
static void
putIndexSet(DbWriter &writer,
            std::vector<uint32_t> &indexes)
{
  std::sort(indexes.begin(), indexes.end());
  writer.putU32(static_cast<uint32_t>(indexes.size()));
  for (uint32_t index : indexes)
    writer.putU32(index);
}

void
DbGraphWriter::writeDelayState()
{
  GraphDelayCalc *dcalc = sta_->graphDelayCalc();
  // A graph can exist with no delays in it, because commands like
  // create_generated_clock levelize. Recording what delay calc had actually
  // done keeps the reader from claiming a graph of zeros is a timed one.
  writer_.putBool(dcalc->delays_seeded_);
  writer_.putBool(dcalc->delays_exist_);

  std::vector<uint32_t> indexes;
  for (Vertex *vertex : dcalc->invalid_delays_)
    indexes.push_back(vertexIndex(vertex));
  putIndexSet(writer_, indexes);

  indexes.clear();
  for (Edge *edge : dcalc->invalid_check_edges_)
    indexes.push_back(edgeIndex(edge));
  putIndexSet(writer_, indexes);

  indexes.clear();
  for (Edge *edge : dcalc->invalid_latch_edges_)
    indexes.push_back(edgeIndex(edge));
  putIndexSet(writer_, indexes);
}

void
DbGraphWriter::writePeriodChecks()
{
  writer_.putU32(static_cast<uint32_t>(graph_->period_check_annotations_.size()));
  for (const auto &[pin, periods] : graph_->period_check_annotations_) {
    writer_.putStr(network_->pathName(pin));
    for (DcalcAPIndex ap = 0; ap < ap_count_; ap++)
      writer_.putF32(periods[ap]);
  }
}

void
DbGraphWriter::write()
{
  if (sta_->variables()->pocvEnabled())
    throw DbUnsupported("stadb cannot store a graph with POCV enabled");
  collect();
  writer_.putU16(static_cast<uint16_t>(ap_count_));
  writeVertices();
  writeEdges();
  writeLevels();
  writeDelayState();
  writePeriodChecks();
}

////////////////////////////////////////////////////////////////

class DbGraphReader
{
public:
  DbGraphReader(DbReader &reader, Sta *sta);
  void read();

private:
  void readVertices();
  void readEdges();
  void readLevels();
  void readDelayState();
  void readPeriodChecks();
  TimingArcSet *getArcSet();
  Pin *getPin();
  Vertex *vertex(uint32_t index) const;
  Edge *edge(uint32_t index) const;

  DbReader &reader_;
  Sta *sta_;
  Graph *graph_;
  Network *network_;
  DcalcAPIndex ap_count_;
  std::vector<Vertex*> vertices_;
  std::vector<Edge*> edges_;
};

DbGraphReader::DbGraphReader(DbReader &reader, Sta *sta) :
  reader_(reader),
  sta_(sta),
  graph_(nullptr),
  network_(sta->network()),
  ap_count_(0)
{
}

Pin *
DbGraphReader::getPin()
{
  std::string_view name = reader_.getStr();
  Pin *pin = network_->findPin(name);
  if (pin == nullptr)
    throw DbCorrupt(sta::format("stadb graph pin {} not found", name));
  return pin;
}

Vertex *
DbGraphReader::vertex(uint32_t index) const
{
  if (index >= vertices_.size())
    throw DbCorrupt("stadb graph vertex index out of range");
  return vertices_[index];
}

Edge *
DbGraphReader::edge(uint32_t index) const
{
  if (index >= edges_.size())
    throw DbCorrupt("stadb graph edge index out of range");
  return edges_[index];
}

TimingArcSet *
DbGraphReader::getArcSet()
{
  if (reader_.getBool())
    return TimingArcSet::wireTimingArcSet();
  std::string_view lib_name = reader_.getStr();
  std::string_view cell_name = reader_.getStr();
  uint32_t index = reader_.getU32();
  LibertyLibrary *library = network_->findLiberty(lib_name);
  LibertyCell *cell = library ? library->findLibertyCell(cell_name) : nullptr;
  if (cell == nullptr)
    throw DbCorrupt(sta::format("stadb graph liberty cell {}/{} not found",
                                lib_name, cell_name));
  const TimingArcSetSeq &arc_sets = cell->timingArcSets();
  if (index >= arc_sets.size())
    throw DbCorrupt(sta::format("stadb graph arc set index {} out of range for {}",
                                index, cell_name));
  return arc_sets[index];
}

void
DbGraphReader::readVertices()
{
  size_t slew_count = RiseFall::index_count * ap_count_;
  size_t count = reader_.getCount("graph vertex");
  vertices_.reserve(count);
  for (size_t i = 0; i < count; i++) {
    Pin *pin = getPin();
    uint8_t flags = reader_.getU8();
    bool is_bidirect_drvr = flags & db_vertex_bidirect_drvr;
    Vertex *vertex = graph_->makeVertex(pin, is_bidirect_drvr,
                                        flags & db_vertex_reg_clk);
    // A bidirect pin owns two vertices, reached through different maps. The
    // load vertex is always created first, so replaying in id order keeps the
    // pair together.
    if (is_bidirect_drvr)
      graph_->pin_bidirect_drvr_vertex_map_[pin] = vertex;
    else
      network_->setVertexId(pin, graph_->id(vertex));
    vertex->setHasChecks(flags & db_vertex_has_checks);
    vertex->setIsCheckClk(flags & db_vertex_is_check_clk);
    vertex->setHasDownstreamClkPin(flags & db_vertex_downstream_clk_pin);
    vertex->setHasSimValue(flags & db_vertex_has_sim_value);
    vertex->setLevel(reader_.getI32());
    for (const MinMax *min_max : MinMax::range()) {
      for (const RiseFall *rf : RiseFall::range()) {
        if (reader_.getBool())
          vertex->setSlewAnnotated(true, rf, min_max->index());
      }
    }
    for (size_t slew_index = 0; slew_index < slew_count; slew_index++) {
      const RiseFall *rf = RiseFall::find(static_cast<int>(slew_index
                                                           % RiseFall::index_count));
      DcalcAPIndex ap = static_cast<DcalcAPIndex>(slew_index
                                                  / RiseFall::index_count);
      graph_->setSlew(vertex, rf, ap, reader_.getF32());
    }
    vertices_.push_back(vertex);
  }
}

void
DbGraphReader::readEdges()
{
  size_t count = reader_.getCount("graph edge");
  edges_.reserve(count);
  for (size_t i = 0; i < count; i++) {
    Vertex *from = vertex(reader_.getU32());
    Vertex *to = vertex(reader_.getU32());
    TimingArcSet *arc_set = getArcSet();
    Edge *edge = graph_->makeEdge(from, to, arc_set);
    uint8_t flags = reader_.getU8();
    edge->setIsBidirectInstPath(flags & db_edge_bidirect_inst_path);
    edge->setIsBidirectNetPath(flags & db_edge_bidirect_net_path);
    edge->setIsBidirectPortPath(flags & db_edge_bidirect_port_path);
    edge->setIsDisabledLoop(flags & db_edge_disabled_loop);
    edge->setHasSimSense(flags & db_edge_sim_sense);
    edge->setHasDisabledCond(flags & db_edge_disabled_cond);
    edge->setDelayAnnotationIsIncremental(flags
                                          & db_edge_incremental_annotation);
    for (const TimingArc *arc : arc_set->arcs()) {
      for (DcalcAPIndex ap = 0; ap < ap_count_; ap++) {
        graph_->setArcDelay(edge, arc, ap, reader_.getF32());
        if (reader_.getBool())
          graph_->setArcDelayAnnotated(edge, arc, ap, true);
      }
    }
    edges_.push_back(edge);
  }
}

void
DbGraphReader::readLevels()
{
  Levelize *levelize = sta_->levelize();
  bool levelized = reader_.getBool();
  levelize->max_level_ = reader_.getI32();
  uint32_t root_count = reader_.getU32();
  for (uint32_t i = 0; i < root_count; i++)
    levelize->roots_.insert(vertex(reader_.getU32()));

  uint32_t loop_count = reader_.getU32();
  for (uint32_t i = 0; i < loop_count; i++) {
    uint32_t edge_count = reader_.getU32();
    EdgeSeq *loop_edges = new EdgeSeq;
    for (uint32_t j = 0; j < edge_count; j++) {
      Edge *loop_edge = edge(reader_.getU32());
      loop_edges->push_back(loop_edge);
      levelize->loop_edges_.insert(loop_edge);
      if (loop_edge->isDisabledLoop())
        levelize->disabled_loop_edges_.insert(loop_edge);
    }
    levelize->loops_.push_back(new GraphLoop(loop_edges));
  }
  // Set last: everything Levelize would have computed is now in place, so
  // ensureLevelized has nothing left to do.
  levelize->levelized_ = levelized;
  levelize->levels_valid_ = levelized;
}

void
DbGraphReader::readDelayState()
{
  GraphDelayCalc *dcalc = sta_->graphDelayCalc();
  // Telling delay calc the delays are already there is what stops the first
  // report from seeding the roots and recomputing everything just restored.
  dcalc->delays_seeded_ = reader_.getBool();
  dcalc->delays_exist_ = reader_.getBool();

  uint32_t count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++)
    dcalc->invalid_delays_.insert(vertex(reader_.getU32()));
  count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++)
    dcalc->invalid_check_edges_.insert(edge(reader_.getU32()));
  count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++)
    dcalc->invalid_latch_edges_.insert(edge(reader_.getU32()));
}

void
DbGraphReader::readPeriodChecks()
{
  uint32_t count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    const Pin *pin = getPin();
    float *periods = new float[ap_count_];
    for (DcalcAPIndex ap = 0; ap < ap_count_; ap++)
      periods[ap] = reader_.getF32();
    graph_->period_check_annotations_[pin] = periods;
  }
}

void
DbGraphReader::read()
{
  if (sta_->graph())
    throw DbCorrupt("stadb graph restore needs a session with no graph");
  ap_count_ = static_cast<DcalcAPIndex>(reader_.getU16());
  if (ap_count_ != sta_->dcalcAnalysisPtCount())
    throw DbCorrupt("stadb graph analysis point count does not match session");
  graph_ = new Graph(sta_, ap_count_);
  graph_->vertices_ = new VertexTable;
  graph_->edges_ = new EdgeTable;
  sta_->graph_ = graph_;
  sta_->updateComponentsState();

  readVertices();
  readEdges();
  readLevels();
  readDelayState();
  readPeriodChecks();
}

////////////////////////////////////////////////////////////////

void
writeStaDbGraph(DbWriter &writer, Sta *sta)
{
  DbGraphWriter graph_writer(writer, sta);
  graph_writer.write();
}

void
readStaDbGraph(DbReader &reader, Sta *sta)
{
  DbGraphReader graph_reader(reader, sta);
  graph_reader.read();
}

} // namespace sta
