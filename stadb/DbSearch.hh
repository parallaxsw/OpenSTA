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

namespace sta {

class DbReader;
class DbWriter;
class Sta;

// Arrival times and the tag pools that give them meaning.
//
// This is the section that makes report_checks free. The graph section alone
// leaves the search to run, and on a large design the search costs about as
// much as everything else in a warm start put together.
//
// Restoring goes through Search's own interning calls -- findClkInfo, findTag,
// setVertexArrivals -- rather than rebuilding the pools by hand. Those calls
// assign the pool indices, maintain the hash sets and keep the tag group
// reference counts, and replaying them in the order the ids were originally
// handed out reproduces the same indices. Paths refer to tags by index, so
// getting that order right is what makes the restored state self consistent.
void writeStaDbSearch(DbWriter &writer, Sta *sta);
void readStaDbSearch(DbReader &reader, Sta *sta);

} // namespace sta
