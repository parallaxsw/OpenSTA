// OpenSTA, Static Timing Analyzer
// Copyright (c) 2025, Parallax Software, Inc.
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

#include "ConcreteLibrary.hh"

#include <cstdlib>
#include <limits>

#include "PatternMatch.hh"
#include "PortDirection.hh"
#include "ParseBus.hh"
#include "ConcreteNetwork.hh"

namespace sta {

using std::string;
using std::map;
using std::min;
using std::max;
using std::abs;
using std::swap;

static constexpr char escape_ = '\\';

ConcreteLibrary::ConcreteLibrary(const char *name,
				 const char *filename,
				 bool is_liberty) :
  name_(name),
  id_(ConcreteNetwork::nextObjectId()),
  filename_(filename ? filename : ""),
  is_liberty_(is_liberty),
  bus_brkt_left_('['),
  bus_brkt_right_(']')
{
}

ConcreteLibrary::~ConcreteLibrary()
{
  cell_map_.deleteContents();
}

ConcreteCell *
ConcreteLibrary::makeCell(const char *name,
			  bool is_leaf,
			  const char *filename)
{
  ConcreteCell *cell = new ConcreteCell(name, filename, is_leaf, this);
  addCell(cell);
  return cell;
}

void
ConcreteLibrary::addCell(ConcreteCell *cell)
{
  cell_map_[cell->name()] = cell;
}

void
ConcreteLibrary::renameCell(ConcreteCell *cell,
			    const char *cell_name)
{
  cell_map_.erase(cell->name());
  cell_map_[cell_name] = cell;
}

void
ConcreteLibrary::deleteCell(ConcreteCell *cell)
{
  cell_map_.erase(cell->name());
  delete cell;
}

ConcreteLibraryCellIterator *
ConcreteLibrary::cellIterator() const
{
  return new ConcreteLibraryCellIterator(cell_map_);
}

ConcreteCell *
ConcreteLibrary::findCell(const char *name) const
{
  return cell_map_.findKey(name);
}

CellSeq
ConcreteLibrary::findCellsMatching(const PatternMatch *pattern) const
{
  CellSeq matches;
  ConcreteLibraryCellIterator cell_iter=ConcreteLibraryCellIterator(cell_map_);
  while (cell_iter.hasNext()) {
    ConcreteCell *cell = cell_iter.next();
    if (pattern->match(cell->name()))
      matches.push_back(reinterpret_cast<Cell*>(cell));
  }
  return matches;
}

void
ConcreteLibrary::setBusBrkts(char left,
			     char right)
{
  bus_brkt_left_ = left;
  bus_brkt_right_ = right;
}

////////////////////////////////////////////////////////////////

ConcreteCell::ConcreteCell(const char *name,
			   const char *filename,
			   bool is_leaf,
                           ConcreteLibrary *library) :
  name_(name),
  id_(ConcreteNetwork::nextObjectId()),
  filename_(filename ? filename : ""),
  library_(library),
  liberty_cell_(nullptr),
  ext_cell_(nullptr),
  port_bit_count_(0),
  is_leaf_(is_leaf)
{
}

ConcreteCell::~ConcreteCell()
{
  ports_.deleteContents();
}

void
ConcreteCell::setName(const char *name)
{
  library_->renameCell(this, name);
  name_ = name;
}

void
ConcreteCell::setLibertyCell(LibertyCell *cell)
{
  liberty_cell_ = cell;
}

void
ConcreteCell::setExtCell(void *ext_cell)
{
  ext_cell_ = ext_cell;
}

ConcretePort *
ConcreteCell::makePort(const char *name)
{
  ConcretePort *port = new ConcretePort(name, false, -1, -1, false, nullptr, this);
  addPort(port);
  return port;
}

ConcretePort *
ConcreteCell::makeBundlePort(const char *name,
			     ConcretePortSeq *members)
{
  ConcretePort *port = new ConcretePort(name, false, -1, -1, true, members, this);
  addPort(port);
  for (ConcretePort *member : *members)
    member->setBundlePort(port);
  return port;
}

ConcretePort *
ConcreteCell::makeBusPort(const char *name,
			  int from_index,
			  int to_index)
{
  ConcretePort *port = new ConcretePort(name, true, from_index, to_index,
					false, new ConcretePortSeq, this);
  addPort(port);
  makeBusPortBits(port, name, from_index, to_index);
  return port;
}

ConcretePort *
ConcreteCell::makeBusPort(const char *name,
			  int from_index,
			  int to_index,
			  ConcretePortSeq *members)
{
  ConcretePort *port = new ConcretePort(name, true, from_index, to_index,
					false, members, this);
  addPort(port);
  return port;
}

void
ConcreteCell::makeBusPortBits(ConcretePort *bus_port,
			      const char *name,
			      int from_index,
			      int to_index)
{
  if (from_index < to_index) {
    for (int index = from_index; index <= to_index; index++)
      makeBusPortBit(bus_port, name, index);
  }
  else {
    for (int index = from_index; index >= to_index; index--)
      makeBusPortBit(bus_port, name, index);
  }
}

void
ConcreteCell::makeBusPortBit(ConcretePort *bus_port,
			     const char *bus_name,
			     int bit_index)
{
  string bit_name;
  stringPrint(bit_name, "%s%c%d%c",
	      bus_name,
	      library_->busBrktLeft(),
	      bit_index,
	      library_->busBrktRight());
  ConcretePort *port = makePort(bit_name.c_str(), bit_index);
  bus_port->addPortBit(port);
  addPortBit(port);
}

ConcretePort *
ConcreteCell::makePort(const char *bit_name,
		       int bit_index)
{
  ConcretePort *port = new ConcretePort(bit_name, false, bit_index,
					bit_index, false, nullptr, this);
  addPortBit(port);
  return port;
}

void
ConcreteCell::addPortBit(ConcretePort *port)
{
  port_map_[port->name()] = port;
  port->setPinIndex(port_bit_count_++);
}

void
ConcreteCell::addPort(ConcretePort *port)
{
  port_map_[port->name()] = port;
  ports_.push_back(port);
  if (!port->hasMembers())
    port->setPinIndex(port_bit_count_++);
}

void
ConcreteCell::setIsLeaf(bool is_leaf)
{
  is_leaf_ = is_leaf;
}

void
ConcreteCell::setAttribute(const string &key,
                           const string &value)
{
  attribute_map_[key] = value;
}

string
ConcreteCell::getAttribute(const string &key) const 
{
  const auto &itr = attribute_map_.find(key);
  if (itr != attribute_map_.end())
    return itr->second;
  return "";
}

ConcretePort *
ConcreteCell::findPort(const char *name) const
{
  return port_map_.findKey(name);
}

size_t
ConcreteCell::portCount() const
{
  return ports_.size();
}

ConcreteCellPortIterator *
ConcreteCell::portIterator() const
{
  return new ConcreteCellPortIterator(ports_);
}

ConcreteCellPortBitIterator *
ConcreteCell::portBitIterator() const
{
  return new ConcreteCellPortBitIterator(this);
}

////////////////////////////////////////////////////////////////

// Helper class for ConcreteCell::groupBusPorts.
class BusPort
{
public:
  BusPort();
  void addBusBit(ConcretePort *port,
                 int index);
  int from() const { return from_; }
  int to() const { return to_; }
  ConcretePortSeq &members() { return members_; }
  const ConcretePortSeq &members() const { return members_; }
  void setDirection(PortDirection *direction);
  PortDirection *direction() const { return direction_; }

private:
  int from_;
  int to_;
  PortDirection *direction_;
  ConcretePortSeq members_;
};

BusPort::BusPort() :
  from_(std::numeric_limits<int>::max()),
  to_(std::numeric_limits<int>::min())
{
}

void
BusPort::setDirection(PortDirection *direction)
{
  direction_ = direction;
}

void
BusPort::addBusBit(ConcretePort *port,
                   int index)
{
  from_ = min(from_, index);
  to_ = max(to_, index);
  members_.push_back(port);
}

void
ConcreteCell::groupBusPorts(const char bus_brkt_left,
			    const char bus_brkt_right,
                            std::function<bool(const char*)> port_msb_first)
{
  const char bus_brkts_left[2]{bus_brkt_left, '\0'};
  const char bus_brkts_right[2]{bus_brkt_right, '\0'};
  map<string, BusPort> bus_map;
  // Find ungrouped bus ports.
  // Remove bus bit ports from the ports_ vector during the scan by
  // keeping an index to the next insertion index and skipping over
  // the ones we want to remove.
  ConcretePortSeq ports = ports_;
  ports_.clear();
  for (ConcretePort *port : ports) {
    const char *port_name = port->name();
    bool is_bus;
    string bus_name;
    int index;
    parseBusName(port_name, bus_brkts_left, bus_brkts_right, escape_,
		 is_bus, bus_name, index);
    if (is_bus) {
      if (!port->isBusBit()) {
        BusPort &bus_port = bus_map[bus_name];
	bus_port.addBusBit(port, index);
        port->setBusBitIndex(index);
        bus_port.setDirection(port->direction());
      }
    }
    else
      ports_.push_back(port);
  }

  // Make the bus ports.
  for (const auto& [bus_name, bus_port] : bus_map) {
    int from = bus_port.from();
    int to = bus_port.to();
    size_t size = to - from + 1;
    bool msb_first = port_msb_first(bus_name.c_str());
    ConcretePortSeq *members = new ConcretePortSeq(size);
    // Index the bus bit ports.
    for (ConcretePort *bus_bit : bus_port.members()) {
      int bit_index = bus_bit->busBitIndex();
      int member_index = msb_first ? to - bit_index : bit_index - from;
      (*members)[member_index] = bus_bit;
    }
    if (msb_first)
      swap(from, to);
    ConcretePort *port = makeBusPort(bus_name.c_str(), from, to, members);
    port->setDirection(bus_port.direction());
  }
}

////////////////////////////////////////////////////////////////

ConcretePort::ConcretePort(const char *name,
			   bool is_bus,
			   int from_index,
			   int to_index,
			   bool is_bundle,
			   ConcretePortSeq *member_ports,
                           ConcreteCell *cell) :
  name_(name),
  id_(ConcreteNetwork::nextObjectId()),
  cell_(cell),
  direction_(PortDirection::unknown()),
  liberty_port_(nullptr),
  ext_port_(nullptr),
  pin_index_(-1),
  is_bundle_(is_bundle),
  is_bus_(is_bus),
  from_index_(from_index),
  to_index_(to_index),
  member_ports_(member_ports),
  bundle_port_(nullptr)
{
}

ConcretePort::~ConcretePort()
{
  // The member ports of a bus are owned by the bus port.
  // The member ports of a bundle are NOT owned by the bus port.
  if (is_bus_)
    member_ports_->deleteContents();
  delete member_ports_;
}

Cell *
ConcretePort::cell() const
{
  return reinterpret_cast<Cell*>(cell_);
}

void
ConcretePort::setLibertyPort(LibertyPort *port)
{
  liberty_port_ = port;
}

void
ConcretePort::setBundlePort(ConcretePort *port)
{
  bundle_port_ = port;
}

void
ConcretePort::setExtPort(void *port)
{
  ext_port_ = port;
}

const char *
ConcretePort::busName() const
{
  if (is_bus_) {
    ConcreteLibrary *lib = cell_->library();
    return stringPrintTmp("%s%c%d:%d%c",
			  name(),
			  lib->busBrktLeft(),
			  from_index_,
			  to_index_,
			  lib->busBrktRight());
  }
  else
    return name();
}

ConcretePort *
ConcretePort::findMember(int index) const
{
  return (*member_ports_)[index];
}

bool
ConcretePort::isBusBit() const
{
  return from_index_ != -1
    && from_index_ == to_index_;
}

void
ConcretePort::setBusBitIndex(int index)
{
  from_index_ = to_index_ = index;
}

int
ConcretePort::size() const
{
  if (is_bus_)
    return abs(to_index_ - from_index_) + 1;
  else if (is_bundle_)
    return static_cast<int>(member_ports_->size());
  else
    return 1;
}

void
ConcretePort::setPinIndex(int index)
{
  pin_index_ = index;
}

void
ConcretePort::setDirection(PortDirection *dir)
{
  direction_ = dir;
  if (hasMembers()) {
    ConcretePortMemberIterator *member_iter = memberIterator();
    while (member_iter->hasNext()) {
      ConcretePort *port_bit = member_iter->next();
      if (port_bit)
        port_bit->setDirection(dir);
    }
    delete member_iter;
  }
}

void
ConcretePort::addPortBit(ConcretePort *port)
{
  member_ports_->push_back(port);
}

ConcretePort *
ConcretePort::findBusBit(int index) const
{
  if (from_index_ < to_index_
      && index <= to_index_
      && index >= from_index_)
    return (*member_ports_)[index - from_index_];
  else if (from_index_ >= to_index_
	   && index >= to_index_
	   && index <= from_index_)
    return (*member_ports_)[from_index_ - index];
  else
    return nullptr;
}

bool
ConcretePort::busIndexInRange(int index) const
{
  return (from_index_ <= to_index_
	  && index <= to_index_
	  && index >= from_index_)
    || (from_index_ > to_index_
	&& index >= to_index_
	&& index <= from_index_);
}

bool
ConcretePort::hasMembers() const
{
  return is_bus_ || is_bundle_;
}

ConcretePortMemberIterator *
ConcretePort::memberIterator() const
{
  return new ConcretePortMemberIterator(member_ports_);
}

////////////////////////////////////////////////////////////////

ConcreteCellPortBitIterator::ConcreteCellPortBitIterator(const ConcreteCell*
							 cell) :
  port_iter_(cell->ports_),
  member_iter_(nullptr),
  next_(nullptr)
{
  findNext();
}

bool
ConcreteCellPortBitIterator::hasNext()
{
  return next_ != nullptr;
}

ConcretePort *
ConcreteCellPortBitIterator::next()
{
  ConcretePort *next = next_;
  findNext();
  return next;
}

void
ConcreteCellPortBitIterator::findNext()
{
  if (member_iter_) {
    if (member_iter_->hasNext()) {
      next_ = member_iter_->next();
      return;
    }
    else {
      delete member_iter_;
      member_iter_ = nullptr;
    }
  }
  while (port_iter_.hasNext()) {
    ConcretePort *next = port_iter_.next();
    if (next->isBus()) {
      member_iter_ = next->memberIterator();
      next_ = member_iter_->next();
      return;
    }
    else if (!next->isBundle()) {
      next_ = next;
      return;
    }
    next_ = nullptr;
  }
  next_ = nullptr;
}

} // namespace
