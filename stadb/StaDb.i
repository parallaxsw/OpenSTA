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

%{
#include "Sta.hh"
#include "stadb/StaDb.hh"
#include "stadb/StaDbCounters.hh"

using sta::Sta;
using sta::StaDbCounters;
using sta::staDbCounters;
using sta::StringSeq;

%}

%inline %{

void
write_sta_db_cmd(const char *filename,
                 bool compress)
{
  sta::writeStaDb(filename, compress, Sta::sta());
}

void
read_sta_db_cmd(const char *filename)
{
  sta::readStaDb(filename, Sta::sta());
}

// Work counters as a flat name/value list, for the regression harness to
// assert that a restore skipped the work rather than repeating it.
StringSeq
sta_db_counters_cmd()
{
  const StaDbCounters &counters = staDbCounters();
  StringSeq values;
  values.push_back("liberty_cells_parsed");
  values.push_back(std::to_string(counters.liberty_cells_parsed));
  values.push_back("graph_vertices_made");
  values.push_back(std::to_string(counters.graph_vertices_made));
  values.push_back("levelize_runs");
  values.push_back(std::to_string(counters.levelize_runs));
  values.push_back("dcalc_vertices_computed");
  values.push_back(std::to_string(counters.dcalc_vertices_computed));
  values.push_back("search_vertices_visited");
  values.push_back(std::to_string(counters.search_vertices_visited));
  values.push_back("spef_nets_parsed");
  values.push_back(std::to_string(counters.spef_nets_parsed));
  return values;
}

void
reset_sta_db_counters_cmd()
{
  staDbCounters() = StaDbCounters();
}

%} // inline
