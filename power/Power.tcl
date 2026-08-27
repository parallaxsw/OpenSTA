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

################################################################
#
# Power commands.
#
################################################################

namespace eval sta {

define_cmd_args "report_power" \
  { [-instances instances]\
      [-exclude_libs patterns]\
      [-highest_power_instances count]\
      [-scene scene]\
      [-digits digits]\
      [-format format]\
      [> filename] [>> filename] } \
  -help {The `report_power` command uses static power analysis based on propagated or annotated pin activities in the circuit using Liberty power models. The internal, switching, leakage and total power are reported. Design power is reported separately for combinational, sequential, macro and pad groups. Power values are reported in watts.

The `read_vcd` or `read_saif` commands can be used to read activities from a file based on simulation. If no simulation activities are available, the `set_power_activity` command should be used to set the activity of input ports or pins in the design. The default input activity and duty for inputs are 0.1 and 0.5 respectively. The activities are propagated from annotated input ports or pins through gates and used in the power calculations.

```
Group                  Internal  Switching    Leakage      Total
                          Power      Power      Power      Power
----------------------------------------------------------------
Sequential             3.29e-06   3.41e-08   2.37e-07   3.56e-06  92.4%
Combinational          1.86e-07   3.31e-08   7.51e-08   2.94e-07   7.6%
Macro                  0.00e+00   0.00e+00   0.00e+00   0.00e+00   0.0%
Pad                    0.00e+00   0.00e+00   0.00e+00   0.00e+00   0.0%
---------------------------------------------------------------
Total                  3.48e-06   6.72e-08   3.12e-07   3.86e-06 100.0%
                          90.2%       1.7%       8.1%
```} \
  -arg_help {
    -instances {`instances`: Report the power for each instance of instances. If the instance is hierarchical the total power for the instances inside the hierarchical instance is reported.}
    -exclude_libs {`patterns`: Report power for each leaf instance whose Liberty library name matches none of patterns.}
    -highest_power_instances {`count`: Report the power for the count highest power instances.}
    -format {`text`: Print a text table (the default). `json`: Print JSON.}
  }

proc_redirect report_power {
  global sta_report_default_digits

  parse_key_args "report_power" args \
    keys {-instances -exclude_libs -highest_power_instances -corner -scene -format -digits}\
    flags {}

  check_argc_eq0 "report_power" $args

  if { ![liberty_libraries_exist] } {
    sta_error 304 "No liberty libraries have been read."
  }
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
    check_positive_integer "-digits" $digits
  } else {
    set digits $sta_report_default_digits
  }
  set scene [parse_scene keys]

  if { [info exists keys(-format)] } {
    set format $keys(-format)
    if { $format != "text" && $format != "json" } {
      sta_error 311 "unknown power report -format $format"
    }
  } else {
    set format "text"
  }

  if { [info exists keys(-instances)] } {
    set insts [get_instances_error "-instances" $keys(-instances)]
    if { $format == "json" } {
      report_power_insts_json $insts $scene $digits
    } else {
      report_power_insts $insts $scene $digits
    }
  } elseif { [info exists keys(-exclude_libs)] } {
    set insts [power_insts_not_in_libs $keys(-exclude_libs)]
    if { $format == "json" } {
      report_power_insts_json $insts $scene $digits
    } else {
      report_power_insts $insts $scene $digits
    }
  } elseif { [info exists keys(-highest_power_instances)] } {
    set count $keys(-highest_power_instances)
    check_positive_integer "-highest_power_instances" $count
    set insts [highest_power_instances $count $scene]
    if { $format == "json" } {
      report_power_insts_json $insts $scene $digits
    } else {
      report_power_insts $insts $scene $digits
    }
  } else {
    if { $format == "json" } {
      report_power_design_json $scene $digits
    } else {
      report_power_design $scene $digits
    }
  }
}

define_cmd_args "report_internal_power_components" { [> filename] [>> filename] } \
  -help {The `report_internal_power_components` command reports internal power broken down by Liberty internal-power components for the current scene.}
proc_redirect report_internal_power_components {
  global sta_report_default_digits
  # Set the default corner
  set scene [cmd_scene]
  if { ![liberty_libraries_exist] } {
    sta_error 304 "No liberty libraries have been read."
  }
  set power_result [internal_power_components $scene]
  report_line $power_result
}


proc liberty_libraries_exist {} {
  set lib_iter [liberty_library_iterator]
  set have_liberty 0
  if { [$lib_iter has_next] } {
    set have_liberty 1
  }
  $lib_iter finish
  return $have_liberty
}

proc report_power_design { corner digits {report_format "text"} } {
  set power_result [design_power $corner]
  set totals        [lrange $power_result  0  3]
  set sequential    [lrange $power_result  4  7]
  set combinational [lrange $power_result  8 11]
  set clock         [lrange $power_result 12 15]
  set macro         [lrange $power_result 16 19]
  set pad           [lrange $power_result 20 end]
  lassign $totals design_internal design_switching design_leakage design_total

  set field_width [max [expr $digits + 6] 10]
  if { $report_format == "text" } {
    report_power_title5       "Group" "Internal" "Switching" "Leakage" "Total" $field_width
    report_power_title5_units "     " "Power"    "Power"     "Power"   "Power" "(Watts)" $field_width
    report_title_dashes5 $field_width
  } elseif { $report_format == "json" } {
    report_line "\{"
  }

  set last_row_var ""
  report_power_row "Sequential"    $sequential    $design_total $field_width $digits $report_format last_row_var
  report_power_row "Combinational" $combinational $design_total $field_width $digits $report_format last_row_var
  report_power_row "Clock"         $clock         $design_total $field_width $digits $report_format last_row_var
  report_power_row "Macro"         $macro         $design_total $field_width $digits $report_format last_row_var
  report_power_row "Pad"           $pad           $design_total $field_width $digits $report_format last_row_var
  if { $report_format == "text" } {
    report_title_dashes5 $field_width
  }
  report_power_row "Total" $power_result $design_total $field_width $digits $report_format last_row_var
  if { $report_format == "text" } {
    report_line "[format %-20s {}][power_col_percent $design_internal  $design_total $field_width][power_col_percent $design_switching $design_total $field_width][power_col_percent $design_leakage $design_total $field_width]"
  } elseif { $report_format == "json" } {
    report_line "\}"
  }
}

proc report_power_design_json { corner digits } {
  set power_result [design_power $corner]
  set totals        [lrange $power_result  0  3]
  set sequential    [lrange $power_result  4  7]
  set combinational [lrange $power_result  8 11]
  set clock         [lrange $power_result 12 15]
  set macro         [lrange $power_result 16 19]
  set pad           [lrange $power_result 20 end]

  report_line "\{"
  report_power_row_json "Sequential" $sequential $digits ","
  report_power_row_json "Combinational" $combinational $digits ","
  report_power_row_json "Clock" $clock $digits ","
  report_power_row_json "Macro" $macro $digits ","
  report_power_row_json "Pad" $pad $digits ","
  report_power_row_json "Total" $totals $digits ""
  report_line "\}"
}

proc report_power_row_json { name row_result digits separator } {
  lassign $row_result internal switching leakage total
  report_line "  \"$name\": \{"
  report_line "    \"internal\": [format %.${digits}e $internal],"
  report_line "    \"switching\": [format %.${digits}e $switching],"
  report_line "    \"leakage\": [format %.${digits}e $leakage],"
  report_line "    \"total\": [format %.${digits}e $total]"
  report_line "  \}$separator"
}

proc max { x y } {
  if { $x >= $y } {
    return $x
  } else {
    return $y
  }
}

proc report_power_title5 { title1 title2 title3 title4 title5 field_width } {
  report_line "[format %-20s $title1] [format %${field_width}s $title2] [format %${field_width}s $title3] [format %${field_width}s $title4] [format %${field_width}s $title5]"
}

proc report_power_title5_units { title1 title2 title3 title4 title5 units field_width } {
  report_line "[format %-20s $title1] [format %${field_width}s $title2] [format %${field_width}s $title3] [format %${field_width}s $title4] [format %${field_width}s $title5] $units"
}

proc report_power_title4 { title1 title2 title3 title4 field_width } {
  report_line " [format %${field_width}s $title1] [format %${field_width}s $title2] [format  %${field_width}s $title3] [format %${field_width}s $title4]"
}

proc report_power_title4_units { title1 title2 title3 title4 units field_width } {
  report_line " [format %${field_width}s $title1] [format %${field_width}s $title2] [format  %${field_width}s $title3] [format %${field_width}s $title4] $units"
}

proc report_title_dashes5 { field_width } {
  set count [expr 20 + ($field_width + 1) * 4]
  report_title_dashes $count
}

proc report_title_dashes4 { field_width } {
  set count [expr ($field_width + 1) * 4]
  report_title_dashes $count
}

proc report_title_dashes { count } {
  set line ""
  for {set i 0} {$i < $count} {incr i} {
    set line "-$line"
  }
  report_line $line
}

proc report_power_row { type row_result design_total field_width digits {report_format "text"} {last_row_var ""} } {
  lassign $row_result internal switching leakage total
  if { $design_total == 0.0 || [is_nan $design_total] } {
    set percent 0.0
  } else {
    set percent [expr $total / $design_total * 100]
  }
  if { "$report_format" == "json" } {
    upvar_opt 1 $last_row_var last_row
    if { $last_row != "" } {
      report_line "  ,"
    }
    report_line "  \"$type\": {"
    report_line "    \"internal\":[power_col $internal $field_width $digits],"
    report_line "    \"switching\":[power_col $switching $field_width $digits],"
    report_line "    \"leakage\":[power_col $leakage $field_width $digits],"
    report_line "    \"total\":[power_col $total $field_width $digits]"
    report_line "  }"
    set last_row $type
  } else {
    report_line "[format %-20s $type][power_col $internal $field_width $digits][power_col $switching $field_width $digits][power_col $leakage $field_width $digits][power_col $total $field_width $digits] [format %5.1f $percent]%"
  }
}

proc power_col { pwr field_width digits } {
  if { [is_nan $pwr] } {
    format " %${field_width}s" $pwr
  } else {
    format " %$field_width.${digits}e" $pwr
  }
}

proc power_col_percent { col_total total field_width } {
  if { $total == 0.0 || [is_nan $total]} {
    set percent 0.0
  } else {
    set percent [expr $col_total / $total * 100]
  }
  format "%$field_width.1f%%" $percent
}

proc report_power_line { type pwr digits } {
  if { [is_nan $pwr] } {
    report_line [format "%-16s %s" $type $pwr]
  } else {
    report_line [format "%-16s %.${digits}e" $type $pwr]
  }
}

proc report_power_insts { insts corner digits {report_format "text"} } {
  set inst_pwrs {}
  foreach_in_collection inst $insts {
    set power_result [instance_power $inst $corner]
    lappend inst_pwrs [list $inst $power_result]
  }
  set inst_pwrs [lsort -command inst_pwr_cmp $inst_pwrs]

  set field_width [max [expr $digits + 6] 10]

  if { $report_format == "text" } {
    report_power_title4       "Internal" "Switching" "Leakage" "Total" $field_width
    report_power_title4_units "Power"    "Power"     "Power"   "Power" "(Watts)" $field_width
    report_title_dashes4 $field_width
  } else {
    report_line {[}
  }

  set last_inst_var ""
  foreach inst_pwr $inst_pwrs {
    set inst [lindex $inst_pwr 0]
    set power [lindex $inst_pwr 1]
    report_power_inst $inst $power $field_width $digits $report_format last_inst_var
  }
  
  if { $report_format == "json" } {
    report_line {]}
  }
}

proc report_power_insts_json { insts corner digits } {
  set inst_pwrs {}
  foreach_in_collection inst $insts {
    set power_result [instance_power $inst $corner]
    lappend inst_pwrs [list $inst $power_result]
  }
  set inst_pwrs [lsort -command inst_pwr_cmp $inst_pwrs]

  report_line "\["
  set first 1
  foreach inst_pwr $inst_pwrs {
    set inst [lindex $inst_pwr 0]
    set power [lindex $inst_pwr 1]
    if { !$first } {
      report_line ","
    }
    set first 0
    report_power_inst_json $inst $power $digits
  }
  report_line "\]"
}

proc report_power_inst_json { inst power digits } {
  lassign $power internal switching leakage total
  set inst_name [get_full_name $inst]
  report_line "\{"
  report_line "  \"name\": \"$inst_name\","
  report_line "  \"internal\": [format %.${digits}e $internal],"
  report_line "  \"switching\": [format %.${digits}e $switching],"
  report_line "  \"leakage\": [format %.${digits}e $leakage],"
  report_line "  \"total\": [format %.${digits}e $total]"
  report_line "\}"
}

proc inst_pwr_cmp { inst_pwr1 inst_pwr2 } {
  set pwr1 [lindex $inst_pwr1 1]
  set pwr2 [lindex $inst_pwr2 1]
  lassign $pwr1 internal1 switching1 leakage1 total1
  lassign $pwr2 internal2 switching2 leakage2 total2
  if { $total1 < $total2 } {
    return 1
  } elseif { $total1 == $total2 } {
    return 0
  } else {
    return -1
  }
}

proc report_power_inst { inst power_result field_width digits {report_format "text"} {last_inst_var ""}} {
  lassign $power_result internal switching leakage total
  if { $report_format == "json" } {
    upvar_opt 1 $last_inst_var last_inst
    if { $last_inst != "" } {
      report_line ","
    }
    report_line "{"
    report_line "  \"name\": \"[get_full_name $inst]\","
    report_line "  \"internal\":[power_col $internal $field_width $digits],"
    report_line "  \"switching\":[power_col $switching $field_width $digits],"
    report_line "  \"leakage\":[power_col $leakage $field_width $digits],"
    report_line "  \"total\":[power_col $total $field_width $digits]"
    report_line "}"
    set last_inst $inst
  } elseif { $report_format == "text" }  {
    report_line "[power_col $internal $field_width $digits][power_col $switching $field_width $digits][power_col $leakage $field_width $digits][power_col $total $field_width $digits] [get_full_name $inst]"
  }
}

################################################################

define_cmd_args "set_power_activity" { [-global]\
                                         [-input]\
                                         [-input_ports ports]\
                                         [-pins pins]\
                                         [-activity activity | -density density]\
                                         [-duty duty]\
                                         [-clock clock]} \
  -help {The `set_power_activity` command is used to set the activity and duty used for power analysis globally or for input ports or pins in the design.

The default input activity for inputs is 0.1 transitions per minimum clock period if a clock is defined or 0.0 if there are no clocks defined. The default input duty is 0.5. This is equivalent to the following command:

```
set_power_activity -input -activity 0.1 -duty 0.5
```} \
  -arg_help {
    -global {Set the activity/duty for all non-clock pins.}
    -input {Set the default input port activity/duty.}
    -input_ports {`input_ports`: Set the input port activity/duty.}
    -pins {`pins`: Set the pin activity/duty.}
    -activity {`activity`: The activity, or number of transitions per clock cycle. If clock is not specified the clock with the minimum period is used. If no clocks are defined an error is reported.}
    -density {`density`: Transitions per library time unit.}
    -duty {`duty`: The duty, or probability the signal is high (0 <= duty <= 1.0). Defaults to 0.5.}
    -clock {`clock`: The clock to use for the period with `-activity`. This option is ignored if `-density` is used.}
  }

proc set_power_activity { args } {
  parse_key_args "set_power_activity" args \
    keys {-input_ports -pins -activity -density -duty -clock} \
    flags {-global -input}

  check_argc_eq0 "set_power_activity" $args
  if { [info exists keys(-activity)] && [info exists keys(-density)] \
         || ![info exists keys(-activity)] && ![info exists keys(-density)] } {
    sta_error 306 "Specify -activity or -density."
  }

  set density 0.0
  if { [info exists keys(-activity)] } {
    set activity $keys(-activity)
    check_positive_float "activity" $activity
    if { [info exists keys(-clock)] } {
      set clk [get_clock_warn "-clock" $keys(-clock)]
    } else {
      set clks [get_clocks]
      if { $clks == {} } {
        sta_error 307 "-activity requires a clock to be defined"
      }
    }
    set density [expr $activity / [clock_min_period [cmd_mode_name]]]
  }

  if { [info exists keys(-density)] } {
    set density $keys(-density)
    check_positive_float "density" $density
    set density [expr $density / [time_ui_sta 1.0]]
    if { [info exists keys(-clock)] } {
      sta_warn 308 "-clock ignored for -density"
    }
  }
  set duty 0.5
  if { [info exists keys(-duty)] } {
    set duty $keys(-duty)
    check_float "duty" $duty
    if { $duty < 0.0 || $duty > 1.0 } {
      sta_error 309 "duty should be 0.0 to 1.0"
    }
  }

  if { [info exists flags(-global)] } {
    set_power_global_activity $density $duty
  }
  if { [info exists flags(-input)] } {
    set_power_input_activity $density $duty
  }
  if { [info exists keys(-input_ports)] } {
    set ports [get_ports_error "input_ports" $keys(-input_ports)]
    foreach port $ports {
      if { [get_property $port "direction"] == "input" || [get_property $port "direction"] == "in" } {
        if { [is_clock_src [get_port_pin $port]] } {
          sta_warn 310 "activity cannot be set on clock ports."
        } else {
          set_power_input_port_activity $port $density $duty
        }
      }
    }
  }
  if { [info exists keys(-pins)] } {
    set pins [get_pins $keys(-pins)]
    foreach pin $pins {
      set_power_pin_activity $pin $density $duty
    }
  }
}

################################################################

define_cmd_args "unset_power_activity" { [-global]\
                                           [-input]\
                                           [-input_ports ports]\
                                           [-pins pins]\
                                           [-clock clock]} \
  -help {The unset_power_activity_command is used to undo the effects of the `set_power_activity` command.} \
  -arg_help {
    -global {Unset the activity/duty for all non-clock pins.}
    -input {Unset the default input port activity/duty.}
    -input_ports {`input_ports`: Unset the input port activity/duty.}
    -pins {`pins`: Unset the pin activity/duty.}
    -clock {`clock`: Unset activity associated with this clock.}
  }

proc unset_power_activity { args } {
  parse_key_args "unset_power_activity" args \
    keys {-input_ports -pins -clock} \
    flags {-global -input}

  check_argc_eq0 "unset_power_activity" $args

  if { [info exists flags(-global)] } {
    unset_power_global_activity
  }
  if { [info exists flags(-input)] } {
    unset_power_input_activity
  }
  if { [info exists keys(-input_ports)] } {
    set ports [get_ports_error "input_ports" $keys(-input_ports)]
    foreach port $ports {
      if { [get_property $port "direction"] == "input" } {
        if { [is_clock_src [get_port_pin $port]] } {
          sta_warn 303 "activity cannot be set on clock ports."
        } else {
          unset_power_input_port_activity $port
        }
      }
    }
  }
  if { [info exists keys(-pins)] } {
    set pins [get_pins $keys(-pins)]
    foreach pin $pins {
      unset_power_pin_activity $pin
    }
  }
}

################################################################

# Deprecated 9/2024
define_cmd_args "read_power_activities" { [-scope scope] -vcd filename } \
  -help {The `read_power_activities` command is deprecated. Use `read_vcd` instead.} \
  -arg_help {
    -scope {The VCD scope of the current design. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.}
    -vcd {VCD file to read. Use `read_vcd` instead.}
    filename {The name of the VCD file to read.}
  }

proc read_power_activities { args } {
  parse_key_args "read_power_activities" args \
    keys {-scope} flags {-vcd}

  check_argc_eq1 "set_power_activity" $args
  set filename [file nativename [lindex $args 0]]
  set scope ""
  if { [info exists keys(-scope)] } {
    set scope $keys(-scope)
  }
  sta_warn 305 "read_power_activities is deprecated. Use read_vcd."
  read_vcd_file $filename $scope [cmd_mode_name] \
    $sta::vcd_null_time $sta::vcd_null_time
}

################################################################

define_cmd_args "read_vcd" \
  {[-scope scope] [-mode mode_name] [-begin_time begin_time] [-end_time end_time] filename} \
  -help {The `read_vcd` command reads a VCD (Value Change Dump) file from a Verilog simulation and extracts pin activities and duty cycles for use in power estimation. Files compressed with gzip are supported. Annotated activities are propagated to the fanout of the annotated pins.} \
  -arg_help {
    -scope {The VCD scope of the current design to extract simulation data. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.}
    -mode {Mode to annotate activities.}
    -begin_time {Ignore VCD activity before this time.}
    -end_time {Ignore VCD activity after this time.}
    filename {The name of the VCD file to read.}
  }

proc read_vcd { args } {
  parse_key_args "read_vcd" args \
    keys {-scope -mode -begin_time -end_time} flags {}

  check_argc_eq1 "read_vcd" $args
  set filename [file nativename [lindex $args 0]]
  set scope ""
  if { [info exists keys(-scope)] } {
    set scope $keys(-scope)
  }

  # Set default start/end times to -1 which means no gating
  set mode_name [cmd_mode_name]
  if { [info exists keys(-mode)] } {
    set mode_name $keys(-mode)
  }
  set begin_time $sta::vcd_null_time
  if { [info exists keys(-begin_time)] } {
    set begin_time $keys(-begin_time)
  }
  set end_time $sta::vcd_null_time
  if { [info exists keys(-end_time)] } {
    set end_time $keys(-end_time)
  }
  read_vcd_file $filename $scope $mode_name $begin_time $end_time
}
################################################################

define_cmd_args "read_saif" { [-scope scope] filename } \
  -help {The `read_saif` command reads a SAIF (Switching Activity Interchange Format) file from a Verilog simulation and extracts pin activities and duty cycles for use in power estimation. Files compressed with gzip are supported. Annotated activities are propagated to the fanout of the annotated pins.} \
  -arg_help {
    -scope {The SAIF scope of the current design to extract simulation data. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.}
    filename {The name of the SAIF file to read.}
  }

proc read_saif { args } {
  parse_key_args "read_saif" args keys {-scope} flags {}
  check_argc_eq1 "read_saif" $args
  set filename [file nativename [lindex $args 0]]
  set scope ""
  if { [info exists keys(-scope)] } {
    set scope $keys(-scope)
  }
  read_saif_file $filename $scope
}

################################################################

define_cmd_args "report_activity_annotation" { [-report_unannotated] \
                                                 [-report_annotated] } \
  -help {Report a summary of pins that are annotated by `read_vcd`, `read_saif` or `set_power_activity`. Sequential internal pins and hierarchical pins are ignored.} \
  -arg_help {
    -report_unannotated {Report unannotated pins.}
    -report_annotated {Report annotated pins.}
  }

proc_redirect report_activity_annotation {
  parse_key_args "report_activity_annotation" args \
    keys {} flags {-report_unannotated -report_annotated}
  check_argc_eq0 "report_activity_annotation" $args
  set report_unannotated [info exists flags(-report_unannotated)]
  set report_annotated [info exists flags(-report_annotated)];
  report_activity_annotation_cmd $report_unannotated $report_annotated
}

################################################################

proc power_find_nan { } {
  set scene [cmd_scene]
  foreach inst [network_leaf_instances] {
    set power_result [instance_power $inst $scene]
    lassign $power_result internal switching leakage total
    if { [is_nan $internal] || [is_nan $switching] || [is_nan $leakage] } {
      report_line "[get_full_name $inst] $internal $switching $leakage"
      break
    }
  }
}

proc is_nan { str } {
  return  [string match "*NaN" $str]
}

# sta namespace end.
}
