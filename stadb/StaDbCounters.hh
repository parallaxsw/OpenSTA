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

#pragma once

#include <cstdint>

namespace sta {

// Counters that prove a .stadb restore skipped work rather than redoing it.
//
// Report diffing cannot catch this class of regression: a restore that
// silently re-parsed liberty, rebuilt the graph, or re-levelized would
// produce byte identical output while delivering none of the speedup. The
// regression harness asserts these stay at zero across a restore.
struct StaDbCounters
{
  uint64_t liberty_cells_parsed = 0;
  uint64_t graph_vertices_made = 0;
  uint64_t levelize_runs = 0;
  uint64_t dcalc_vertices_computed = 0;
  uint64_t search_vertices_visited = 0;
  uint64_t spef_nets_parsed = 0;
};

// Process wide, since the counters span sessions within one process.
StaDbCounters &staDbCounters();

} // namespace sta
