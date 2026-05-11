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

#include <cstddef>
#include <string>
#include <string_view>

#include "NetworkClass.hh"
#include "SearchClass.hh"

namespace sta {

// External producers register extensions to add custom columns to
// report_checks output. ReportPath takes ownership of the registered
// pointer and deletes it at shutdown.
class ReportFieldExtension
{
 public:
  virtual ~ReportFieldExtension() = default;

  // Identifier matched against `-fields` tokens.
  virtual std::string_view name() const = 0;
  // Column header.
  virtual std::string_view title() const = 0;
  // Column width in characters.
  virtual size_t width() const = 0;

  // Per-row value. Return "" to leave the cell blank.
  virtual std::string value(const Path *path,
                            const Pin *pin,
                            const Instance *inst) const = 0;
};

}  // namespace sta
