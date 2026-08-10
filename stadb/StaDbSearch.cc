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

#include "StaDbSearch.hh"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ExceptionPath.hh"
#include "Format.hh"
#include "Graph.hh"
#include "Network.hh"
#include "Path.hh"
#include "PortDelay.hh"
#include "Scene.hh"
#include "Sdc.hh"
#include "Search.hh"
#include "Sta.hh"
#include "StaDbCodec.hh"
#include "StaDbSections.hh"
#include "search/ClkInfo.hh"
#include "search/Sim.hh"
#include "search/Tag.hh"
#include "search/TagGroup.hh"

namespace sta {

// Clk infos and tags reference each other: a tag names its clk info, and a clk
// info that carries CRPR names the tag of the clock path it came from. The two
// pools are therefore one stream of tagged records emitted in dependency
// order, so a reader replaying it front to back always already has the operand
// it needs. This terminates because the path a clk info points at is a clock
// path, and clock tags carry no CRPR path.
enum class DbSearchKind : uint8_t {
  clk_info = 1,
  tag = 2,
};

// How a ClkInfo's uncertainties pointer was obtained. Pointer identity takes
// part in ClkInfo equality, so it has to come back pointing at the same Sdc
// owned object rather than at an equal copy.
enum class DbUncertaintiesKind : uint8_t {
  none = 0,
  clock = 1,
  pin = 2,
};

// Deferred previous path link. A path's predecessor usually lives on a vertex
// that has not been rebuilt yet, and the arrays move as they are built, so the
// links are resolved in a second pass once every array is in place.
struct DbPrevRec
{
  Tag *tag;
  Tag *prev_tag;
  uint32_t vertex;
  uint32_t prev_vertex;
  uint32_t prev_edge;
  uint8_t prev_arc;
};

////////////////////////////////////////////////////////////////

class DbSearchWriter
{
public:
  DbSearchWriter(DbWriter &writer, Sta *sta);
  void write();

private:
  void collect();
  void writeTagPool();
  bool tagIsWritable(const Tag *tag) const;
  uint32_t writeVertexPaths(DbWriter &paths);
  void writeFlags();
  void writeSim();
  uint32_t clkInfoId(const ClkInfo *clk_info);
  // Pair ordinal. Tags are interned two at a time, one per transition, and
  // both halves share every field except the transition itself.
  uint32_t tagId(const Tag *tag);
  void putTagRef(DbWriter &out, const Tag *tag);
  void writeClkInfo(const ClkInfo *clk_info);
  void writeTag(const Tag *tag);
  void putPin(DbWriter &out, const Pin *pin);
  void putClkEdge(const ClockEdge *clk_edge);
  void putUncertainties(const ClkInfo *clk_info);
  void putExceptionStates(const Tag *tag);
  uint32_t vertexIndex(const Vertex *vertex) const;
  uint32_t edgeIndex(const Edge *edge) const;

  DbWriter &writer_;
  // Pool records are emitted as a side effect of asking for an id while
  // walking the paths, so they accumulate in their own buffer and are spliced
  // in ahead of the path records they are referenced by.
  DbWriter pool_;
  Sta *sta_;
  Search *search_;
  Graph *graph_;
  const Network *network_;
  Sdc *sdc_;
  std::vector<Vertex*> vertices_;
  std::unordered_map<const Vertex*, uint32_t> vertex_indexes_;
  std::unordered_map<const Edge*, uint32_t> edge_indexes_;
  std::unordered_map<const ClkInfo*, uint32_t> clk_info_ids_;
  std::unordered_map<const Tag*, uint32_t> tag_ids_;
  std::unordered_map<const ExceptionPath*, uint32_t> exception_ids_;
  std::unordered_set<const void*> in_progress_;
  uint32_t pool_count_ = 0;
};

DbSearchWriter::DbSearchWriter(DbWriter &writer, Sta *sta) :
  writer_(writer),
  pool_(writer.strings()),
  sta_(sta),
  search_(sta->search()),
  graph_(sta->graph()),
  network_(sta->network()),
  sdc_(sta->cmdSdc())
{
}

void
DbSearchWriter::collect()
{
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext())
    vertices_.push_back(vertex_iter.next());
  // The same dense numbering the graph section uses, recomputed the same way
  // so neither section has to store a mapping for the other.
  std::sort(vertices_.begin(), vertices_.end(),
            [this](const Vertex *a, const Vertex *b) {
              return graph_->id(a) < graph_->id(b);
            });
  std::vector<Edge*> edges;
  for (uint32_t i = 0; i < vertices_.size(); i++) {
    vertex_indexes_[vertices_[i]] = i;
    VertexOutEdgeIterator edge_iter(vertices_[i], graph_);
    while (edge_iter.hasNext())
      edges.push_back(edge_iter.next());
  }
  std::sort(edges.begin(), edges.end(),
            [this](const Edge *a, const Edge *b) {
              return graph_->id(a) < graph_->id(b);
            });
  for (uint32_t i = 0; i < edges.size(); i++)
    edge_indexes_[edges[i]] = i;

  // Exceptions are named by their position in the same content sorted order
  // the sdc section writes them in, which both sides can recompute. Their Sdc
  // ids are no use here: those are handed out in command order, and a restored
  // session numbers from scratch.
  std::vector<ExceptionPath*> sorted(sdc_->exceptions().begin(),
                                     sdc_->exceptions().end());
  sort(sorted, ExceptionPathLess(network_));
  for (uint32_t i = 0; i < sorted.size(); i++)
    exception_ids_[sorted[i]] = i;
}

uint32_t
DbSearchWriter::vertexIndex(const Vertex *vertex) const
{
  auto itr = vertex_indexes_.find(vertex);
  if (itr == vertex_indexes_.end())
    throw DbCorrupt("stadb search references an unknown vertex");
  return itr->second;
}

uint32_t
DbSearchWriter::edgeIndex(const Edge *edge) const
{
  auto itr = edge_indexes_.find(edge);
  if (itr == edge_indexes_.end())
    throw DbCorrupt("stadb search references an unknown edge");
  return itr->second;
}

void
DbSearchWriter::putPin(DbWriter &out, const Pin *pin)
{
  out.putBool(pin != nullptr);
  if (pin)
    out.putStr(network_->pathName(pin));
}

void
DbSearchWriter::putClkEdge(const ClockEdge *clk_edge)
{
  pool_.putBool(clk_edge != nullptr);
  if (clk_edge) {
    pool_.putStr(clk_edge->clock()->name());
    pool_.putU8(static_cast<uint8_t>(clk_edge->transition()->index()));
  }
}

void
DbSearchWriter::putUncertainties(const ClkInfo *clk_info)
{
  const ClockUncertainties *uncertainties = clk_info->uncertainties();
  if (uncertainties == nullptr)
    pool_.putU8(static_cast<uint8_t>(DbUncertaintiesKind::none));
  else {
    // Only two things own one: the clock itself, and a set_clock_uncertainty
    // on a pin.
    const Clock *clk = clk_info->clock();
    const Pin *clk_src = clk_info->clkSrc();
    if (clk && uncertainties == &clk->uncertainties()) {
      pool_.putU8(static_cast<uint8_t>(DbUncertaintiesKind::clock));
      pool_.putStr(clk->name());
    }
    else if (clk_src && sdc_->clockUncertainties(clk_src) == uncertainties) {
      pool_.putU8(static_cast<uint8_t>(DbUncertaintiesKind::pin));
      pool_.putStr(network_->pathName(clk_src));
    }
    else
      throw DbUnsupported("stadb search clock uncertainties have no owner");
  }
}

void
DbSearchWriter::writeClkInfo(const ClkInfo *clk_info)
{
  // Resolved before the record opens, because asking for the tag id can emit
  // records of its own and they have to land ahead of this one.
  const Path *crpr_clk_path = clk_info->crprClkPathRaw();
  uint32_t crpr_vertex = 0;
  uint32_t crpr_tag = 0;
  uint8_t crpr_rf = 0;
  if (crpr_clk_path) {
    crpr_vertex = vertexIndex(crpr_clk_path->vertex(sta_));
    crpr_tag = tagId(crpr_clk_path->tag(sta_));
    crpr_rf = static_cast<uint8_t>(crpr_clk_path->tag(sta_)->rfIndex());
  }

  pool_.putU8(static_cast<uint8_t>(DbSearchKind::clk_info));
  putClkEdge(clk_info->clkEdge());
  putPin(pool_, clk_info->clkSrc());
  pool_.putBool(clk_info->isPropagated());
  putPin(pool_, clk_info->genClkSrc());
  pool_.putBool(clk_info->isGenClkSrcPath());
  pool_.putI32(clk_info->isPulseClk() ? clk_info->pulseClkSenseRfIndex() : -1);
  pool_.putF32(delayAsFloat(clk_info->insertion()));
  pool_.putF32(clk_info->latency());
  putUncertainties(clk_info);
  pool_.putU8(static_cast<uint8_t>(clk_info->minMaxIndex()));
  pool_.putBool(crpr_clk_path != nullptr);
  if (crpr_clk_path) {
    pool_.putU32(crpr_vertex);
    pool_.putU32(crpr_tag);
    pool_.putU8(crpr_rf);
  }
}

void
DbSearchWriter::putExceptionStates(const Tag *tag)
{
  ExceptionStateSet *states = tag->states();
  pool_.putU32(states ? static_cast<uint32_t>(states->size()) : 0);
  if (states) {
    // Sorted so that a hash set's iteration order does not leak into the file.
    std::vector<std::pair<uint32_t, int32_t>> refs;
    for (ExceptionState *state : *states) {
      auto itr = exception_ids_.find(state->exception());
      if (itr == exception_ids_.end())
        throw DbUnsupported(sta::format("stadb search tag references an "
                                        "exception that is not in the sdc: {}",
                                        state->exception()->to_string(network_)));
      refs.emplace_back(itr->second, state->index());
    }
    std::sort(refs.begin(), refs.end());
    for (const auto &[exception_id, state_index] : refs) {
      pool_.putU32(exception_id);
      pool_.putI32(state_index);
    }
  }
}

void
DbSearchWriter::writeTag(const Tag *tag)
{
  uint32_t clk_info_id = clkInfoId(tag->clkInfo());

  pool_.putU8(static_cast<uint8_t>(DbSearchKind::tag));
  pool_.putU32(clk_info_id);
  pool_.putU8(static_cast<uint8_t>(tag->minMaxIndex()));
  pool_.putBool(tag->isClock());
  InputDelay *input_delay = tag->inputDelay();
  pool_.putI32(input_delay ? input_delay->index() : -1);
  pool_.putBool(tag->isSegmentStart());
  putExceptionStates(tag);
}

// Ids number the records in the order their bytes land in the stream, which is
// the order the reader replays them. Writing recurses into dependencies first,
// so an id can only be claimed once the record it names is complete.
uint32_t
DbSearchWriter::clkInfoId(const ClkInfo *clk_info)
{
  auto itr = clk_info_ids_.find(clk_info);
  if (itr != clk_info_ids_.end())
    return itr->second;
  if (!in_progress_.insert(clk_info).second)
    throw DbUnsupported("stadb search clk info and tag reference a cycle");
  writeClkInfo(clk_info);
  in_progress_.erase(clk_info);
  uint32_t id = pool_count_++;
  clk_info_ids_[clk_info] = id;
  return id;
}

uint32_t
DbSearchWriter::tagId(const Tag *tag)
{
  auto itr = tag_ids_.find(tag);
  if (itr != tag_ids_.end())
    return itr->second;
  if (!in_progress_.insert(tag).second)
    throw DbUnsupported("stadb search clk info and tag reference a cycle");
  writeTag(tag);
  in_progress_.erase(tag);
  uint32_t id = pool_count_++;
  // Both halves of the pair answer to the same ordinal, since one record
  // recreates both.
  TagIndex base = tag->index() & ~static_cast<TagIndex>(1);
  for (const RiseFall *rf : RiseFall::range()) {
    Tag *pair_tag = search_->tag(base + rf->index());
    if (pair_tag)
      tag_ids_[pair_tag] = id;
  }
  return id;
}

void
DbSearchWriter::putTagRef(DbWriter &out, const Tag *tag)
{
  out.putU32(tagId(tag));
  out.putU8(static_cast<uint8_t>(tag->rfIndex()));
}

// Generated clock source paths are found through a filter exception that is
// built from the clock rather than from the sdc, so it is not in the file and
// its tags cannot be named. Those tags are left behind by that search and no
// vertex path refers to them, so dropping them changes nothing.
bool
DbSearchWriter::tagIsWritable(const Tag *tag) const
{
  ExceptionStateSet *states = tag->states();
  if (states) {
    for (ExceptionState *state : *states) {
      if (!exception_ids_.contains(state->exception()))
        return false;
    }
  }
  return true;
}

// Every writable tag in the pool, in the order the indices were handed out,
// rather than only the ones some path points at. Emitting the pool whole is
// what lets a restored session write back a byte identical file: a tag the
// restore did not recreate would silently vanish from the next write.
void
DbSearchWriter::writeTagPool()
{
  for (TagIndex i = 0; i < search_->tag_next_; i += RiseFall::index_count) {
    // Holes come from filtered queries, which delete a pair at a time.
    Tag *tag = search_->tag(i);
    if (tag && tagIsWritable(tag))
      tagId(tag);
  }
}

uint32_t
DbSearchWriter::writeVertexPaths(DbWriter &paths)
{
  uint32_t vertex_count = 0;
  for (Vertex *vertex : vertices_) {
    TagGroup *tag_group = search_->tagGroup(vertex);
    if (tag_group) {
      vertex_count++;
      paths.putU32(vertexIndex(vertex));
      paths.putU32(static_cast<uint32_t>(tag_group->pathCount()));
      VertexPathIterator path_iter(vertex, sta_);
      while (path_iter.hasNext()) {
        Path *path = path_iter.next();
        putTagRef(paths, path->tag(sta_));
        paths.putF32(delayAsFloat(path->arrival()));
        paths.putF32(delayAsFloat(path->required()));
        paths.putBool(path->isEnum());
        Path *prev = path->prevPath();
        paths.putBool(prev != nullptr);
        if (prev) {
          paths.putU32(vertexIndex(prev->vertex(sta_)));
          putTagRef(paths, prev->tag(sta_));
          paths.putU32(edgeIndex(path->prevEdge(sta_)));
          paths.putU8(static_cast<uint8_t>(path->prevArc(sta_)->index()));
        }
      }
    }
  }
  return vertex_count;
}

void
DbSearchWriter::writeFlags()
{
  writer_.putBool(search_->arrivals_exist_);
  writer_.putBool(search_->arrivals_seeded_);
  writer_.putBool(search_->clk_arrivals_valid_);
  writer_.putBool(search_->requireds_exist_);
  writer_.putBool(search_->requireds_seeded_);
  writer_.putBool(search_->found_downstream_clk_pins_);
}

void
DbSearchWriter::writeSim()
{
  Sim *sim = sta_->modes()[0]->sim();
  writer_.putBool(sim->valid_);
  writer_.putBool(sim->incremental_);
  writer_.putBool(sim->const_func_pins_valid_);

  // Sorted by name throughout: these are hash containers, so their iteration
  // order follows allocation addresses and would break byte idempotence.
  std::vector<std::pair<std::string, uint8_t>> pin_values;
  for (const auto &[pin, value] : sim->simValues())
    pin_values.emplace_back(network_->pathName(pin), static_cast<uint8_t>(value));
  std::sort(pin_values.begin(), pin_values.end());
  writer_.putU32(static_cast<uint32_t>(pin_values.size()));
  for (const auto &[name, value] : pin_values) {
    writer_.putStr(name);
    writer_.putU8(value);
  }

  std::vector<std::string> const_pins;
  for (const Pin *pin : sim->const_func_pins_)
    const_pins.push_back(network_->pathName(pin));
  std::sort(const_pins.begin(), const_pins.end());
  writer_.putU32(static_cast<uint32_t>(const_pins.size()));
  for (const std::string &name : const_pins)
    writer_.putStr(name);

  std::vector<uint32_t> disabled;
  for (const Edge *edge : sim->edge_disabled_cond_set_)
    disabled.push_back(edgeIndex(edge));
  std::sort(disabled.begin(), disabled.end());
  writer_.putU32(static_cast<uint32_t>(disabled.size()));
  for (uint32_t index : disabled)
    writer_.putU32(index);

  std::vector<std::pair<uint32_t, uint8_t>> senses;
  for (const auto &[edge, sense] : sim->edge_timing_sense_map_)
    senses.emplace_back(edgeIndex(edge), static_cast<uint8_t>(sense));
  std::sort(senses.begin(), senses.end());
  writer_.putU32(static_cast<uint32_t>(senses.size()));
  for (const auto &[index, sense] : senses) {
    writer_.putU32(index);
    writer_.putU8(sense);
  }
}

void
DbSearchWriter::write()
{
  if (sta_->modes().size() != 1)
    throw DbUnsupported("stadb search supports a single mode");
  // A path filter is per query state that the next report would throw away
  // anyway, so dropping it here costs nothing and keeps filter tags, which
  // reference exceptions that are not written, out of the file.
  search_->deleteFilteredArrivals();
  collect();

  writeTagPool();
  DbWriter paths(writer_.strings());
  uint32_t vertex_count = writeVertexPaths(paths);
  writer_.putU32(pool_count_);
  writer_.putBytes(pool_.bytes().data(), pool_.size());
  writer_.putU32(vertex_count);
  writer_.putBytes(paths.bytes().data(), paths.size());
  writeFlags();
  writeSim();
}

////////////////////////////////////////////////////////////////

class DbSearchReader
{
public:
  DbSearchReader(DbReader &reader, Sta *sta);
  void read();

private:
  void collect();
  void readPool();
  void readClkInfo();
  void readTag();
  void readVertexPaths();
  void linkPrevPaths();
  void readFlags();
  void readSim();
  const Pin *getPin();
  Pin *getPinNonConst();
  const ClockEdge *getClkEdge();
  const ClockUncertainties *getUncertainties();
  ExceptionStateSet *getExceptionStates();
  Vertex *vertex(uint32_t index) const;
  Edge *edge(uint32_t index) const;
  Tag *getTagRef();
  Tag *tag(uint32_t id, uint32_t rf_index) const;
  const ClkInfo *clkInfo(uint32_t id) const;

  DbReader &reader_;
  Sta *sta_;
  Search *search_;
  Graph *graph_;
  Network *network_;
  Sdc *sdc_;
  Scene *scene_;
  std::vector<Vertex*> vertices_;
  std::vector<Edge*> edges_;
  std::vector<ExceptionPath*> exceptions_;
  std::vector<InputDelay*> input_delays_;
  // One entry per pool record, in emission order. Only one of the two is set.
  // A tag entry holds the rise half of the pair; the fall half sits at the
  // next pool index.
  std::vector<Tag*> tags_;
  std::vector<const ClkInfo*> clk_infos_;
  std::vector<DbPrevRec> prev_recs_;
};

DbSearchReader::DbSearchReader(DbReader &reader, Sta *sta) :
  reader_(reader),
  sta_(sta),
  search_(sta->search()),
  graph_(sta->graph()),
  network_(sta->network()),
  sdc_(sta->cmdSdc()),
  scene_(sta->scenes()[0])
{
}

void
DbSearchReader::collect()
{
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext())
    vertices_.push_back(vertex_iter.next());
  std::sort(vertices_.begin(), vertices_.end(),
            [this](const Vertex *a, const Vertex *b) {
              return graph_->id(a) < graph_->id(b);
            });
  for (Vertex *v : vertices_) {
    VertexOutEdgeIterator edge_iter(v, graph_);
    while (edge_iter.hasNext())
      edges_.push_back(edge_iter.next());
  }
  std::sort(edges_.begin(), edges_.end(),
            [this](const Edge *a, const Edge *b) {
              return graph_->id(a) < graph_->id(b);
            });

  exceptions_.assign(sdc_->exceptions().begin(), sdc_->exceptions().end());
  sort(exceptions_, ExceptionPathLess(network_));

  for (InputDelay *input_delay : sdc_->inputDelays()) {
    size_t index = input_delay->index();
    if (input_delays_.size() <= index)
      input_delays_.resize(index + 1, nullptr);
    input_delays_[index] = input_delay;
  }
}

Vertex *
DbSearchReader::vertex(uint32_t index) const
{
  if (index >= vertices_.size())
    throw DbCorrupt("stadb search vertex index out of range");
  return vertices_[index];
}

Edge *
DbSearchReader::edge(uint32_t index) const
{
  if (index >= edges_.size())
    throw DbCorrupt("stadb search edge index out of range");
  return edges_[index];
}

Tag *
DbSearchReader::tag(uint32_t id,
                    uint32_t rf_index) const
{
  if (id >= tags_.size() || tags_[id] == nullptr)
    throw DbCorrupt("stadb search tag id out of range");
  Tag *rise = tags_[id];
  Tag *tag = search_->tag(rise->index() + rf_index);
  if (tag == nullptr)
    throw DbCorrupt("stadb search tag transition is missing from the pool");
  return tag;
}

Tag *
DbSearchReader::getTagRef()
{
  uint32_t id = reader_.getU32();
  return tag(id, reader_.getU8());
}

const ClkInfo *
DbSearchReader::clkInfo(uint32_t id) const
{
  if (id >= clk_infos_.size() || clk_infos_[id] == nullptr)
    throw DbCorrupt("stadb search clk info id out of range");
  return clk_infos_[id];
}

const Pin *
DbSearchReader::getPin()
{
  if (!reader_.getBool())
    return nullptr;
  std::string_view name = reader_.getStr();
  const Pin *pin = network_->findPin(name);
  if (pin == nullptr)
    throw DbCorrupt(sta::format("stadb search pin {} not found", name));
  return pin;
}

Pin *
DbSearchReader::getPinNonConst()
{
  return const_cast<Pin*>(getPin());
}

const ClockEdge *
DbSearchReader::getClkEdge()
{
  if (!reader_.getBool())
    return nullptr;
  std::string_view name = reader_.getStr();
  const RiseFall *rf = RiseFall::find(static_cast<int>(reader_.getU8()));
  Clock *clk = sdc_->findClock(name);
  if (clk == nullptr)
    throw DbCorrupt(sta::format("stadb search clock {} not found", name));
  return clk->edge(rf);
}

const ClockUncertainties *
DbSearchReader::getUncertainties()
{
  DbUncertaintiesKind kind = static_cast<DbUncertaintiesKind>(reader_.getU8());
  switch (kind) {
  case DbUncertaintiesKind::none:
    return nullptr;
  case DbUncertaintiesKind::clock: {
    std::string_view name = reader_.getStr();
    Clock *clk = sdc_->findClock(name);
    if (clk == nullptr)
      throw DbCorrupt(sta::format("stadb search clock {} not found", name));
    return &clk->uncertainties();
  }
  case DbUncertaintiesKind::pin: {
    std::string_view name = reader_.getStr();
    const Pin *pin = network_->findPin(name);
    if (pin == nullptr)
      throw DbCorrupt(sta::format("stadb search pin {} not found", name));
    return sdc_->clockUncertainties(pin);
  }
  }
  throw DbCorrupt("stadb search unknown clock uncertainties kind");
}

void
DbSearchReader::readClkInfo()
{
  const ClockEdge *clk_edge = getClkEdge();
  const Pin *clk_src = getPin();
  bool is_propagated = reader_.getBool();
  const Pin *gen_clk_src = getPin();
  bool is_gen_clk_src_path = reader_.getBool();
  int32_t pulse_sense = reader_.getI32();
  const RiseFall *pulse_clk_sense =
      pulse_sense < 0 ? nullptr : RiseFall::find(pulse_sense);
  float insertion = reader_.getF32();
  float latency = reader_.getF32();
  const ClockUncertainties *uncertainties = getUncertainties();
  const MinMax *min_max = MinMax::find(static_cast<int>(reader_.getU8()));

  // A locator, not a path: ClkInfo resolves it back to the live path on the
  // vertex through Path::vertexPath, so only the vertex and tag are read.
  Path crpr_clk_path;
  Path *crpr_clk_path_ptr = nullptr;
  if (reader_.getBool()) {
    Vertex *crpr_vertex = vertex(reader_.getU32());
    Tag *crpr_tag = getTagRef();
    crpr_clk_path.init(crpr_vertex, crpr_tag, sta_);
    crpr_clk_path_ptr = &crpr_clk_path;
  }
  clk_infos_.push_back(search_->findClkInfo(scene_, clk_edge, clk_src,
                                            is_propagated, gen_clk_src,
                                            is_gen_clk_src_path,
                                            pulse_clk_sense, insertion, latency,
                                            uncertainties, min_max,
                                            crpr_clk_path_ptr));
  tags_.push_back(nullptr);
}

ExceptionStateSet *
DbSearchReader::getExceptionStates()
{
  uint32_t count = reader_.getU32();
  if (count == 0)
    return nullptr;
  ExceptionStateSet *states = new ExceptionStateSet;
  for (uint32_t i = 0; i < count; i++) {
    uint32_t exception_id = reader_.getU32();
    int32_t state_index = reader_.getI32();
    if (exception_id >= exceptions_.size())
      throw DbCorrupt("stadb search exception id out of range");
    ExceptionState *state = exceptions_[exception_id]->firstState();
    while (state && state->index() != state_index)
      state = state->nextState();
    if (state == nullptr)
      throw DbCorrupt("stadb search exception state index out of range");
    states->insert(state);
  }
  return states;
}

void
DbSearchReader::readTag()
{
  const ClkInfo *clk_info = clkInfo(reader_.getU32());
  const MinMax *min_max = MinMax::find(static_cast<int>(reader_.getU8()));
  bool is_clk = reader_.getBool();
  int32_t input_delay_index = reader_.getI32();
  InputDelay *input_delay = nullptr;
  if (input_delay_index >= 0) {
    if (static_cast<size_t>(input_delay_index) >= input_delays_.size()
        || input_delays_[input_delay_index] == nullptr)
      throw DbCorrupt("stadb search input delay index out of range");
    input_delay = input_delays_[input_delay_index];
  }
  bool is_segment_start = reader_.getBool();
  ExceptionStateSet *states = getExceptionStates();
  // Asking for the rise half interns both, which is what keeps a restored
  // pool laid out the same way the original one was.
  tags_.push_back(search_->findTag(scene_, RiseFall::rise(), min_max, clk_info,
                                   is_clk, input_delay, is_segment_start,
                                   states, states != nullptr, nullptr));
  clk_infos_.push_back(nullptr);
}

void
DbSearchReader::readPool()
{
  uint32_t count = reader_.getU32();
  tags_.reserve(count);
  clk_infos_.reserve(count);
  for (uint32_t i = 0; i < count; i++) {
    DbSearchKind kind = static_cast<DbSearchKind>(reader_.getU8());
    switch (kind) {
    case DbSearchKind::clk_info:
      readClkInfo();
      break;
    case DbSearchKind::tag:
      readTag();
      break;
    default:
      throw DbCorrupt("stadb search unknown pool record kind");
    }
  }
}

void
DbSearchReader::readVertexPaths()
{
  uint32_t vertex_count = reader_.getU32();
  for (uint32_t i = 0; i < vertex_count; i++) {
    uint32_t vertex_index = reader_.getU32();
    Vertex *v = vertex(vertex_index);
    uint32_t path_count = reader_.getU32();
    // Going through TagGroupBldr rather than filling the array directly keeps
    // the tag group interning, index assignment and reference counting in the
    // one place that already knows how to do it.
    TagGroupBldr bldr(true, sta_);
    bldr.init(v);
    std::vector<std::pair<Tag*, float>> requireds;
    std::vector<Tag*> enum_tags;
    for (uint32_t j = 0; j < path_count; j++) {
      Tag *path_tag = getTagRef();
      float arrival = reader_.getF32();
      float required = reader_.getF32();
      bool is_enum = reader_.getBool();
      bldr.insertPath(path_tag, arrival, nullptr, nullptr, nullptr);
      requireds.emplace_back(path_tag, required);
      if (is_enum)
        enum_tags.push_back(path_tag);
      if (reader_.getBool()) {
        DbPrevRec rec;
        rec.tag = path_tag;
        rec.vertex = vertex_index;
        rec.prev_vertex = reader_.getU32();
        rec.prev_tag = getTagRef();
        rec.prev_edge = reader_.getU32();
        rec.prev_arc = reader_.getU8();
        prev_recs_.push_back(rec);
      }
    }
    search_->setVertexArrivals(v, &bldr);
    // insertPath only carries the tag, arrival and previous path, so the rest
    // is set on the array it produced.
    for (const auto &[path_tag, required] : requireds)
      Path::vertexPath(v, path_tag, sta_)->setRequired(required);
    for (Tag *enum_tag : enum_tags)
      Path::vertexPath(v, enum_tag, sta_)->setIsEnum(true);
  }
}

void
DbSearchReader::linkPrevPaths()
{
  for (const DbPrevRec &rec : prev_recs_) {
    Path *path = Path::vertexPath(vertex(rec.vertex), rec.tag, sta_);
    Path *prev = Path::vertexPath(vertex(rec.prev_vertex), rec.prev_tag, sta_);
    if (path == nullptr || prev == nullptr)
      throw DbCorrupt("stadb search previous path does not resolve");
    Edge *prev_edge = edge(rec.prev_edge);
    TimingArc *prev_arc = prev_edge->timingArcSet()->findTimingArc(rec.prev_arc);
    // Order matters: the previous edge id shares storage with the vertex id,
    // and which one is live is decided by the previous path being set.
    path->setPrevPath(prev);
    path->setPrevEdgeArc(prev_edge, prev_arc, sta_);
  }
}

void
DbSearchReader::readFlags()
{
  search_->arrivals_exist_ = reader_.getBool();
  search_->arrivals_seeded_ = reader_.getBool();
  search_->clk_arrivals_valid_ = reader_.getBool();
  search_->requireds_exist_ = reader_.getBool();
  search_->requireds_seeded_ = reader_.getBool();
  search_->found_downstream_clk_pins_ = reader_.getBool();
}

void
DbSearchReader::readSim()
{
  // Without this the first report would re-propagate constants, and setting a
  // pin value that search has not seen before invalidates every arrival that
  // was just restored.
  Sim *sim = sta_->modes()[0]->sim();
  sim->valid_ = reader_.getBool();
  sim->incremental_ = reader_.getBool();
  sim->const_func_pins_valid_ = reader_.getBool();

  uint32_t count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    std::string_view name = reader_.getStr();
    const Pin *pin = network_->findPin(name);
    if (pin == nullptr)
      throw DbCorrupt(sta::format("stadb search sim pin {} not found", name));
    sim->sim_value_map_[pin] = static_cast<LogicValue>(reader_.getU8());
  }

  count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    std::string_view name = reader_.getStr();
    const Pin *pin = network_->findPin(name);
    if (pin == nullptr)
      throw DbCorrupt(sta::format("stadb search sim pin {} not found", name));
    sim->const_func_pins_.insert(pin);
  }

  count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++)
    sim->edge_disabled_cond_set_.insert(edge(reader_.getU32()));

  count = reader_.getU32();
  for (uint32_t i = 0; i < count; i++) {
    Edge *sense_edge = edge(reader_.getU32());
    sim->edge_timing_sense_map_[sense_edge] =
        static_cast<TimingSense>(reader_.getU8());
  }
}

void
DbSearchReader::read()
{
  if (graph_ == nullptr)
    throw DbCorrupt("stadb search restore needs a graph");
  if (sta_->modes().size() != 1)
    throw DbUnsupported("stadb search supports a single mode");
  collect();
  readPool();
  readVertexPaths();
  linkPrevPaths();
  readFlags();
  readSim();
}

////////////////////////////////////////////////////////////////

void
writeStaDbSearch(DbWriter &writer, Sta *sta)
{
  DbSearchWriter search_writer(writer, sta);
  search_writer.write();
}

void
readStaDbSearch(DbReader &reader, Sta *sta)
{
  DbSearchReader search_reader(reader, sta);
  search_reader.read();
}

} // namespace sta
