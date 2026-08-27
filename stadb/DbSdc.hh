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

namespace sta {

class DbReader;
class DbWriter;
class Sta;

// The SDC section is a tagged record stream rather than a struct dump. The
// constraint kinds have almost nothing in common with each other, and new ones
// appear upstream regularly, so a stream that a reader can extend one tag at a
// time drifts more gracefully than one wide record.
//
// Values are stored as raw floats, never as text. Writing constraints as SDC
// and re-sourcing them looks appealing because WriteSdc already covers every
// command, but it is measurably lossy: a clock waveform edge of 1.3 comes back
// as a float two ulps away even at 17 digits, because the value is scaled into
// display units, printed, parsed and scaled back. That shifts reported arrival
// times, which defeats the point of a cache that must be indistinguishable
// from a cold run.
enum class DbSdcKind : uint8_t {
  end = 0,
  analysis_type,
  operating_conditions,
  voltage,
  net_voltage,
  instance_pvt,
  wireload,
  wireload_mode,
  wireload_selection,
  clock,
  generated_clock,
  clock_slew,
  clock_uncertainty,
  clock_slew_limit,
  propagated_clock_pin,
  pin_uncertainty,
  inter_clock_uncertainty,
  clock_latency,
  clock_insertion,
  clock_groups,
  clock_sense,
  clock_gating_check,
  input_delay,
  output_delay,
  exception,
  group_path,
  data_check,
  disable_pin,
  disable_port,
  disable_lib_port,
  disable_cell_ports,
  disable_inst_ports,
  disable_gating_check,
  logic_value,
  case_value,
  port_ext_cap,
  net_wire_cap,
  net_resistance,
  input_drive,
  derating_global,
  derating_net,
  derating_inst,
  derating_cell,
  slew_limit_port,
  slew_limit_cell,
  cap_limit_port,
  cap_limit_pin,
  cap_limit_cell,
  fanout_limit_port,
  fanout_limit_cell,
  latch_borrow_limit,
  min_pulse_width,
  max_area,
  max_dynamic_power,
  max_leakage_power,
  max_lol,
};

void writeStaDbSdc(DbWriter &writer, Sta *sta);
void readStaDbSdc(DbReader &reader, Sta *sta);

} // namespace sta
