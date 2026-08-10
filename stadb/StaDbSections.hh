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
#include <string>

#include "Error.hh"
#include "StaDbCodec.hh"

namespace sta {

// Thrown by the writer when the session holds state this format cannot
// represent. Distinct from DbCorrupt: the caller should skip caching rather
// than regenerate, because regenerating would hit the same limitation.
class DbUnsupported : public Exception
{
public:
  explicit DbUnsupported(const std::string &msg);
  const char *what() const noexcept override;

private:
  std::string msg_;
};

////////////////////////////////////////////////////////////////
//
// Liberty section records.
//
// Enums that have name conversion functions upstream (TimingType, TimingRole)
// are stored by name so that inserting an enumerator upstream cannot silently
// reinterpret old files. Enums without name conversions are stored as small
// integers and range checked on read.

// A pool reference. Zero means null, so stored ids are pool index + 1.
using DbPoolId = uint32_t;

#define STADB_REC_TABLE_MODEL(X)  \
  X(u32, table)                   \
  X(sid, tbl_template)            \
  X(u8, template_type)            \
  X(u8, scale_factor_type)        \
  X(u8, rf_index)

STADB_RECORD(DbTableModelRec, STADB_REC_TABLE_MODEL)

#define STADB_REC_CELL(X)               \
  X(sid, name)                          \
  X(sid, filename)                      \
  X(f32, area)                          \
  X(b, dont_use)                        \
  X(b, is_macro)                        \
  X(b, is_memory)                       \
  X(b, is_pad)                          \
  X(b, is_clock_cell)                   \
  X(b, is_level_shifter)                \
  X(u8, level_shifter_type)             \
  X(b, is_isolation_cell)               \
  X(b, always_on)                       \
  X(u8, switch_cell_type)               \
  X(b, interface_timing)                \
  X(u8, clock_gate_type)                \
  X(b, has_infered_reg_timing_arcs)     \
  X(f32, ocv_arc_depth)                 \
  X(sid, footprint)                     \
  X(sid, user_function_class)           \
  X(b, leakage_power_exists)            \
  X(f32, leakage_power)

STADB_RECORD(DbCellRec, STADB_REC_CELL)

#define STADB_REC_PORT(X)          \
  X(sid, direction)                \
  X(u8, pwr_gnd_type)              \
  X(sid, voltage_name)             \
  X(u8, scan_signal_type)          \
  X(f32, fanout_load)              \
  X(b, fanout_load_exists)         \
  X(f32, min_period)               \
  X(b, is_clk)                     \
  X(b, is_reg_clk)                 \
  X(b, is_check_clk)               \
  X(b, is_reg_output)              \
  X(b, is_latch_data)              \
  X(b, is_latch_output)            \
  X(b, is_clk_gate_clk)            \
  X(b, is_clk_gate_enable)         \
  X(b, is_clk_gate_out)            \
  X(b, is_pll_feedback)            \
  X(b, is_switch)                  \
  X(b, is_pad)                     \
  X(b, isolation_cell_data)        \
  X(b, isolation_cell_enable)      \
  X(b, level_shifter_data)         \
  X(u8, pulse_clk_trigger)         \
  X(u8, pulse_clk_sense)

STADB_RECORD(DbPortRec, STADB_REC_PORT)

#define STADB_REC_ARC_SET(X)  \
  X(u32, from_port)           \
  X(u32, to_port)             \
  X(u32, related_out_port)    \
  X(sid, role)                \
  X(sid, timing_type)         \
  X(u8, timing_sense)         \
  X(sid, sdf_cond)            \
  X(sid, sdf_cond_start)      \
  X(sid, sdf_cond_end)        \
  X(sid, mode_name)           \
  X(sid, mode_value)          \
  X(f32, ocv_arc_depth)

STADB_RECORD(DbArcSetRec, STADB_REC_ARC_SET)

// Presence bits for the five optional models a TableModels bundle can hold.
constexpr uint8_t stadb_models_model = 1 << 0;
constexpr uint8_t stadb_models_sigma_early = 1 << 1;
constexpr uint8_t stadb_models_sigma_late = 1 << 2;
constexpr uint8_t stadb_models_std_dev = 1 << 3;
constexpr uint8_t stadb_models_mean_shift = 1 << 4;
constexpr uint8_t stadb_models_skewness = 1 << 5;
// Early and late sigma may be the same object, which the destructor relies on.
constexpr uint8_t stadb_models_sigma_aliased = 1 << 6;

// Tag distinguishing the concrete TimingModel subclass on a timing arc.
enum class DbModelKind : uint8_t {
  none = 0,
  gate_table = 1,
  check_table = 2,
  gate_linear = 3,
  check_linear = 4,
};

// Tag for FuncExpr nodes, mirroring FuncExpr::Op.
enum class DbFuncKind : uint8_t {
  null = 0,
  port = 1,
  not_ = 2,
  or_ = 3,
  and_ = 4,
  xor_ = 5,
  one = 6,
  zero = 7,
};

// Tag for the port structure of a liberty cell. Shared with the network
// section, whose cell ports have the same three shapes.
enum class DbPortKind : uint8_t {
  scalar = 0,
  bus = 1,
  bundle = 2,
};

////////////////////////////////////////////////////////////////
//
// Network section records.
//
// Network objects reference each other by dense index into this section's own
// arrays. ObjectId cannot serve as the reference: it comes from a process wide
// counter that is never reset, so a restored session hands out different ids
// than the session that wrote the file.
//
// Each array is ordered by ascending ObjectId, which is creation order. That
// matters beyond determinism: a net holds its pins and terms in a singly
// prepended list, so replaying creation order is what makes the restored lists
// iterate the same way, and it also guarantees a referent is always built
// before the object that names it.

// Index into a network section array. Unlike DbPoolId, zero is a valid index.
using DbNetworkId = uint32_t;
constexpr DbNetworkId db_network_id_null = 0xffffffffu;

#define STADB_REC_INSTANCE(X)  \
  X(sid, name)                 \
  X(u32, cell)                 \
  X(u32, parent)

STADB_RECORD(DbInstanceRec, STADB_REC_INSTANCE)

#define STADB_REC_NET(X)  \
  X(sid, name)            \
  X(u32, instance)        \
  X(u32, merged_into)     \
  X(u8, constant)

STADB_RECORD(DbNetRec, STADB_REC_NET)

// A pin and the term that bridges it to the level below, which upstream always
// creates as a pair, so one record covers both.
#define STADB_REC_PIN(X)  \
  X(u32, instance)        \
  X(i32, pin_index)       \
  X(u32, net)             \
  X(b, has_term)          \
  X(u32, term_net)

STADB_RECORD(DbPinRec, STADB_REC_PIN)

// Constant net markers. Offset by one so that zero means "not a constant".
constexpr uint8_t stadb_constant_none = 0;
constexpr uint8_t stadb_constant_zero = 1;
constexpr uint8_t stadb_constant_one = 2;

} // namespace sta
