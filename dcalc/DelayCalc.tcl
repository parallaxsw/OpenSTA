# OpenSTA, Static Timing Analyzer
# Copyright (c) 2026, Parallax Software, Inc.
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

define_cmd_args "report_dcalc" \
  {[-from from_pin] [-to to_pin] [-scene scene] [-min] [-max] [-digits digits]} \
  -help {The `report_dcalc` command shows how the delays between instance pins are calculated. It is useful for debugging problems with delay calculation.} \
  -arg_help {
    -from {Report delay calculations for timing arcs from instance input pin from_pin.}
    -to {Report delay calculations for timing arcs to instance output pin to_pin.}
    -scene {Report delay calculations for this scene. Required if more than one scene is defined.}
    -min default
    -max default
    -digits default
  }

proc_redirect report_dcalc {
  report_dcalc_cmd "report_dcalc" $args "-digits"
}

# Allow any combination of -from/-to pins.
proc report_dcalc_cmd { cmd cmd_args digits_key } {
  global sta_report_default_digits

  parse_key_args $cmd cmd_args \
    keys "$digits_key -from -to -scene" \
    flags {-min -max}
  set scene [parse_scene keys]
  set min_max [parse_min_max_flags flags]
  check_argc_eq0 $cmd $cmd_args

  set digits $sta_report_default_digits
  if [info exists keys($digits_key)] {
    set digits $keys($digits_key)
    check_positive_integer $digits_key $digits
  }
  if {[info exists keys(-from)] && [info exists keys(-to)]} {
    set from_pin [get_port_pin_error "from_pin" $keys(-from)]
    set to_pin [get_port_pin_error "to_pin" $keys(-to)]
    foreach from_vertex [$from_pin vertices] {
      foreach to_vertex [$to_pin vertices] {
        set iter [$from_vertex out_edge_iterator]
        while {[$iter has_next]} {
          set edge [$iter next]
          if { [$edge to] == $to_vertex } {
            report_edge_dcalc $edge $scene $min_max $digits
          }
        }
        $iter finish
      }
    }
  } elseif [info exists keys(-from)] {
    set from_pin [get_port_pin_error "from_pin" $keys(-from)]
    foreach from_vertex [$from_pin vertices] {
      set iter [$from_vertex out_edge_iterator]
      while {[$iter has_next]} {
        set edge [$iter next]
        report_edge_dcalc $edge $scene $min_max $digits
      }
      $iter finish
    }
  } elseif [info exists keys(-to)] {
    set to_pin [get_port_pin_error "to_pin" $keys(-to)]
    foreach to_vertex [$to_pin vertices] {
      set iter [$to_vertex in_edge_iterator]
      while {[$iter has_next]} {
        set edge [$iter next]
        report_edge_dcalc $edge $scene $min_max $digits
      }
      $iter finish
    }
  }
}

proc report_edge_dcalc { edge scene min_max digits } {
  set role [$edge role]
  if { $role != "wire" } {
    set from_vertex [$edge from]
    set from_pin [$from_vertex pin]
    set to_vertex [$edge to]
    set to_pin [$to_vertex pin]
    set cell [[$to_pin instance] cell]
    set library [$cell library]
    # Filter timing checks based on min_max.
    if {!(($min_max == "max" && $role == "hold") \
            || ($min_max=="min" && $role=="setup"))} {
      report_line "Library: [get_name $library]"
      report_line "Cell: [get_name $cell]"
      set sense [$edge sense]
      if { $sense != "unknown" } {
        report_line "Arc sense: $sense"
      }
      report_line "Arc type: $role"

      foreach arc [$edge timing_arcs] {
        set from [get_name [$from_pin port]]
        set from_rf [$arc from_edge]
        set to [get_name [$to_pin port]]
        set to_rf [$arc to_edge]
        report_line "$from $from_rf -> $to $to_rf"
        report_line [report_delay_calc_cmd $edge $arc $scene $min_max $digits]
        if { [$edge delay_annotated $arc $scene $min_max] } {
          set delay [$edge arc_delay $arc $scene $min_max]
          report_line "Annotated value = [format_time $delay $digits]"
        }
        report_line "............................................."
        report_line ""
      }
    }
  }
}

################################################################

define_hidden_cmd_args "set_delay_calculator" [delay_calc_names]

proc set_delay_calculator { alg } {
  if { [is_delay_calc_name $alg] } {
    set_delay_calculator_cmd $alg
  } else {
    sta_error 2500 "delay calculator $alg not found."
  }
}

################################################################

define_cmd_args "report_slews" {[-scenes scenes] [-digits digits]\
                                  [-report_variance] pin} \
  -help {Report the slews at a pin.} \
  -arg_help {
    -scenes {Report slews for these scenes. The default is all scenes.}
    -report_variance {Report SSTA distribution parameters.}
    pin {A pin or port.}
    -digits default
  }

define_cmd_alias "report_slew" "report_slews"

proc report_slews { args } {
  global sta_report_default_digits

  parse_key_args "report_slews" args keys {-scenes -digits} \
    flags {-report_variance}
  check_argc_eq1 "report_slews" $args

  set scenes [parse_scenes_or_all keys]
  set pin [get_port_pin_error "pin" [lindex $args 0]]
  if [info exists keys(-digits)] {
    set digits $keys(-digits)
    check_positive_integer "-digits" $digits
  } else {
    set digits $sta_report_default_digits
  }
  set report_variance [info exists flags(-report_variance)]

  foreach vertex [$pin vertices] {
    set rise_min [$vertex slew_scenes_string rise $scenes min $report_variance $digits]
    set rise_max [$vertex slew_scenes_string rise $scenes max $report_variance $digits]
    set fall_min [$vertex slew_scenes_string fall $scenes min $report_variance $digits]
    set fall_max [$vertex slew_scenes_string fall $scenes max $report_variance $digits]
    report_line "[vertex_path_name $vertex] [rise_short_name] $rise_min:$rise_max [fall_short_name] $fall_min:$fall_max"
  }
}

# sta namespace end
}
