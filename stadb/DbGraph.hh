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

// The timing graph, its levels, and the delays and slews annotated onto it.
//
// Vertices and edges are renumbered to dense ids on write. ObjectTable has no
// allocate-at-id call, and its ids develop holes once anything is deleted, so
// the only way to land an object on a chosen id is to hand out ids in order
// from an empty table. Writing in ascending id order and replaying creation in
// that same order does exactly that, and it has a second payoff: makeEdge
// prepends to the vertex edge lists, so replaying in creation order rebuilds
// those lists in the original order without touching the links by hand.
void writeStaDbGraph(DbWriter &writer, Sta *sta);
void readStaDbGraph(DbReader &reader, Sta *sta);

} // namespace sta
