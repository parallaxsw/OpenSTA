# OpenSTA, Static Timing Analyzer
# Copyright (c) 2026, Silimate, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
# 
# The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software.
# 
# Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
# 
# This notice may not be removed or altered from any source distribution.

namespace eval sta {

define_cmd_args "write_sta_db" {[-no_compress] filename}

proc write_sta_db { args } {
  parse_key_args "write_sta_db" args keys {} flags {-no_compress}
  check_argc_eq1 "write_sta_db" $args
  set filename [file nativename [lindex $args 0]]
  write_sta_db_cmd $filename [expr ![info exists flags(-no_compress)]]
}

define_cmd_args "read_sta_db" {filename}

proc read_sta_db { args } {
  parse_key_args "read_sta_db" args keys {} flags {}
  check_argc_eq1 "read_sta_db" $args
  set filename [file nativename [lindex $args 0]]
  if { ![file readable $filename] } {
    sta_error 2740 "$filename is not readable."
  }
  read_sta_db_cmd $filename
}

# namespace
}
