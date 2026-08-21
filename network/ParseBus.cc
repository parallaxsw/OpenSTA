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

#include "ParseBus.hh"

#include <cctype>
#include <string>
#include <string_view>

#include "StringUtil.hh"

namespace sta {

namespace {

bool
allDigits(std::string_view s)
{
  if (s.empty())
    return false;
  size_t i = 0;
  if (s[0] == '-' || s[0] == '+') {
    if (s.size() == 1)
      return false;
    i = 1;
  }
  for (; i < s.size(); i++) {
    if (!std::isdigit(static_cast<unsigned char>(s[i])))
      return false;
  }
  return true;
}

// Text after the left bus bracket, including the closing bracket.
// Returns false for glob residue such as "*]*sbc[*]".
bool
parseBusInside(std::string_view inside,
               int &from,
               int &to,
               bool &is_range,
               bool &subscript_wild)
{
  is_range = false;
  subscript_wild = false;
  if (inside.empty())
    return false;
  char last = inside.back();
  if (last == ']' || last == '}' || last == ')')
    inside = inside.substr(0, inside.size() - 1);
  if (inside.empty())
    return false;
  if (inside == "*") {
    subscript_wild = true;
    return true;
  }
  size_t colon = inside.find(':');
  if (colon != std::string_view::npos) {
    std::string_view from_str = inside.substr(0, colon);
    std::string_view to_str = inside.substr(colon + 1);
    if (!allDigits(from_str) || !allDigits(to_str))
      return false;
    is_range = true;
    from = std::stoi(std::string(from_str));
    to = std::stoi(std::string(to_str));
    return true;
  }
  if (!allDigits(inside))
    return false;
  from = to = std::stoi(std::string(inside));
  return true;
}

} // namespace

bool
isBusName(std::string_view name,
          const char brkt_left,
          const char brkt_right,
          char escape)
{
  size_t len = name.size();
  // Shortest bus name is a[0].
  if (len >= 4
      // Escaped bus brackets are not buses.
      && name[len - 2] != escape
      && name[len - 1] == brkt_right) {
    size_t left = name.rfind(brkt_left);
    return left != std::string_view::npos;
  }
  else
    return false;
}

void
parseBusName(std::string_view name,
             const char brkt_left,
             const char brkt_right,
             const char escape,
             // Return values.
             bool &is_bus,
             std::string &bus_name,
             int &index)
{
  parseBusName(name, std::string_view(&brkt_left, 1),
               std::string_view(&brkt_right, 1), escape,
               is_bus, bus_name, index);
}

void
parseBusName(std::string_view name,
             std::string_view brkts_left,
             std::string_view brkts_right,
             char escape,
             // Return values.
             bool &is_bus,
             std::string &bus_name,
             int &index)
{
  is_bus = false;
  size_t len = name.size();
  // Shortest bus name is a[0].
  if (len >= 4
      // Escaped bus brackets are not buses.
      && name[len - 2] != escape) {
    char last_ch = name[len - 1];
    size_t brkt_index = brkts_right.find(last_ch);
    if (brkt_index != std::string_view::npos) {
      char brkt_left_ch = brkts_left[brkt_index];
      size_t left = name.rfind(brkt_left_ch);
      if (left != std::string_view::npos) {
        int from = 0;
        int to = 0;
        bool is_range = false;
        bool subscript_wild = false;
        // Ranges and [*] are not a single index; leave is_bus false
        // so callers fall back to glob matching.
        if (parseBusInside(name.substr(left + 1), from, to, is_range,
                           subscript_wild)
            && !is_range && !subscript_wild) {
          is_bus = true;
          bus_name.append(name.substr(0, left));
          index = from;
        }
      }
    }
  }
}

void
parseBusName(std::string_view name,
             const char brkt_left,
             const char brkt_right,
             char escape,
             // Return values.
             bool &is_bus,
             bool &is_range,
             std::string &bus_name,
             int &from,
             int &to,
             bool &subscript_wild)
{
  parseBusName(name, std::string_view(&brkt_left, 1),
               std::string_view(&brkt_right, 1), escape,
               is_bus, is_range, bus_name, from, to, subscript_wild);
}

void
parseBusName(std::string_view name,
             std::string_view brkts_left,
             std::string_view brkts_right,
             char escape,
             // Return values.
             bool &is_bus,
             bool &is_range,
             std::string &bus_name,
             int &from,
             int &to,
             bool &subscript_wild)
{
  is_bus = false;
  is_range = false;
  subscript_wild = false;
  size_t len = name.size();
  // Shortest bus is a[0].
  if (len >= 4
      // Escaped bus brackets are not buses.
      && name[len - 2] != escape) {
    char last_ch = name[len - 1];
    size_t brkt_index = brkts_right.find(last_ch);
    if (brkt_index != std::string_view::npos) {
      char brkt_left_ch = brkts_left[brkt_index];
      size_t left = name.rfind(brkt_left_ch);
      if (left != std::string_view::npos) {
        if (parseBusInside(name.substr(left + 1), from, to, is_range,
                           subscript_wild)) {
          is_bus = true;
          bus_name.append(name.substr(0, left));
        }
      }
    }
  }
}

std::string
escapeChars(std::string_view token,
            char ch1,
            char ch2,
            char ch3,
            char escape)
{
  std::string escaped;
  escaped.reserve(token.size());
  for (size_t i = 0; i < token.size(); i++) {
    char ch = token[i];
    if (ch == escape) {
      if (i + 1 < token.size()) {
        escaped += ch;
        escaped += token[i + 1];
        i++;
      }
      else
        escaped += ch;
    }
    else if (ch == ch1 || ch == ch2 || ch == ch3) {
      escaped += escape;
      escaped += ch;
    }
    else
      escaped += ch;
  }
  return escaped;
}

} // namespace sta
