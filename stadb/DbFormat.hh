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

#pragma once

#include <cstdint>

#include "Graph.hh"
#include "Liberty.hh"
#include "TableModel.hh"
#include "TimingArc.hh"

namespace sta {

// "STDB" in little endian byte order.
constexpr uint32_t stadb_magic = 0x42445453;

// Bump on any incompatible layout change that sizeof alone does not catch,
// such as reordering fields within a record or changing an encoding.
constexpr uint32_t stadb_version = 1;

// v1 supports exactly one scene. Kept as a stored field so that adding corners
// later is a version bump rather than a format redesign.
constexpr uint32_t stadb_scene_count = 1;

enum class DbSectionId : uint32_t {
  strings = 1,
  liberty = 2,
  network = 3,
  sdc = 4,
  parasitics = 5,
  graph = 6,
  search = 7,
};

constexpr uint32_t stadb_flag_compressed = 1 << 0;

// Largest expansion deflate can achieve, used to reject a section header that
// claims a decompressed size its stored bytes could not produce.
constexpr uint64_t stadb_max_inflate_ratio = 1032;

// Hard cap on a decompressed section. Large liberties can be tens of
// gigabytes; this is still far above that, so a ratio-valid header that names
// an impossible payload is rejected before the section is allocated.
constexpr uint64_t stadb_max_section_bytes = 1ull << 36;

// Whole-file cap applied before the container vector is allocated. Stored
// payloads cannot exceed the decompressed sizes already refused above, and
// v1 writes a handful of sections.
constexpr uint64_t stadb_max_file_bytes = stadb_max_section_bytes * 8;

// Sanity bound on a bus port's bit count, which is generated from an index
// range rather than from bytes in the file. Far above any real bus.
constexpr int64_t stadb_max_bus_width = 1 << 20;

////////////////////////////////////////////////////////////////
//
// ABI guard.
//
// Graph vertices and edges are the only structures written field by field, and
// their layouts are composed purely of pointers, fixed width integers and
// bitfields, so their sizes are stable across any LP64 platform. Asserting them
// turns an upstream field addition into a build failure that a human has to
// look at, which is the whole point of the guard.
//
// The liberty and network classes are written through public accessors rather
// than by layout, so their sizes cannot be asserted portably: std::map and
// friends differ between libc++ and libstdc++. They still feed the guard hash
// below, because a size change there means upstream added state that this
// format does not yet know about, and stale caches should be discarded.

static_assert(sizeof(Vertex) == 48,
              "Vertex layout changed; review stadb graph section and bump "
              "stadb_version");
static_assert(sizeof(Edge) == 48,
              "Edge layout changed; review stadb graph section and bump "
              "stadb_version");

constexpr uint64_t
stadbAbiMix(uint64_t hash, uint64_t value)
{
  hash ^= value;
  hash *= 1099511628211ull;
  return hash;
}

// Derived from sizeof rather than hand maintained, so any layout drift in a
// serialized class automatically invalidates existing files. A mismatch is
// treated as a cache miss, never as an error.
constexpr uint64_t
stadbAbiGuard()
{
  uint64_t hash = 14695981039346656037ull;
  hash = stadbAbiMix(hash, sizeof(Vertex));
  hash = stadbAbiMix(hash, sizeof(Edge));
  hash = stadbAbiMix(hash, sizeof(Graph));
  hash = stadbAbiMix(hash, sizeof(LibertyLibrary));
  hash = stadbAbiMix(hash, sizeof(LibertyCell));
  hash = stadbAbiMix(hash, sizeof(LibertyPort));
  hash = stadbAbiMix(hash, sizeof(TimingArcSet));
  hash = stadbAbiMix(hash, sizeof(TimingArc));
  hash = stadbAbiMix(hash, sizeof(TableModel));
  hash = stadbAbiMix(hash, sizeof(GateTableModel));
  hash = stadbAbiMix(hash, sizeof(CheckTableModel));
  hash = stadbAbiMix(hash, sizeof(Table));
  hash = stadbAbiMix(hash, sizeof(TableAxis));
  hash = stadbAbiMix(hash, stadb_version);
  return hash;
}

} // namespace sta
