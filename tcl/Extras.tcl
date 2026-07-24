################################################################
# Helpers for path end reporting
################################################################

namespace eval sta {

proc get_paths { args } {
  global sta_report_unconstrained_paths
  parse_report_path_options "get_paths" args "full" 0
  return [find_timing_paths_cmd "get_paths" args]
}

proc_redirect report_paths {
  report_path_ends {*}$args
}

define_cmd_args "report_echo" {message}

proc_redirect report_echo {
  parse_key_args "report_echo" args \
    keys {} flags {}
  check_argc_eq1 "report_echo" $args
  
  set message [lindex $args 0]
  report_line "$message"
}

# Dump target PPA to JSON file
proc target_ppa_json { filepath } {
  set ppa_json [open "$filepath" "w"]

  # Retrieve max_logic_levels from global scope (set by user constraints)
  global max_logic_levels
  if { ![info exists max_logic_levels] } { # default to 0 if undefined
    set max_logic_levels 0
  }
  
  # Dump target PPA to JSON file
  puts $ppa_json "{"
  puts $ppa_json "  \"max_area\": [get_max_area],"
  puts $ppa_json "  \"max_dynamic_power\": [get_max_dynamic_power],"
  puts $ppa_json "  \"max_leakage_power\": [get_max_leakage_power],"
  puts $ppa_json "  \"max_logic_levels\": $max_logic_levels"
  puts $ppa_json "}"
  close $ppa_json
}

}

################################################################
# Miscellaneous commands
################################################################

sta::define_cmd_args "set_dont_use" {lib_cell_name_pattern}
     
proc set_dont_use {lib_cell_name_pattern} {
  set targets [get_lib_cells -filter "name=~$lib_cell_name_pattern"]
  foreach_in_collection target $targets {
    $target set_dont_use
  }
}

sta::define_cmd_args "unset_dont_use" {lib_cell_name_pattern}
     
proc unset_dont_use {lib_cell_name_pattern} {
  set targets [get_lib_cells -filter "name=~$lib_cell_name_pattern"]
  foreach_in_collection target $targets {
    $target unset_dont_use
  }
}

sta::define_cmd_args "get_flat_pins" {arg}

proc get_flat_pins {arg} {
  return [get_pins -hier -filter "is_hierarchical==false && full_name=~$arg"]
}

sta::define_cmd_args "get_flat_cells" {arg}

proc get_flat_cells {arg} {
  return [get_cells -hier -filter "is_hierarchical==false && full_name=~$arg"]
}

# Set dont_touch attribute (ignore/to be implemented)
interp alias {} set_dont_touch {} return -level 0

# Set dont_touch_network attribute (ignore/to be implemented)
interp alias {} set_dont_touch_network {} return -level 0

# Get object name
interp alias {} get_object_name {} get_full_name

# Query objects (ignore/to be implemented)
interp alias {} query_objects {} return -level 0

# remove/reset aliases for "unset" commands in sdc.tcl
interp alias {} remove_output_delay {} unset_output_delay
interp alias {} reset_output_delay {} unset_output_delay

interp alias {} remove_input_delay {} unset_input_delay
interp alias {} reset_input_delay {} unset_input_delay

interp alias {} remove_propagated_clock {} unset_propagated_clock
interp alias {} reset_propagated_clock {} unset_propagated_clock

interp alias {} remove_clock_groups {} unset_clock_groups
interp alias {} reset_clock_groups {} unset_clock_groups

interp alias {} remove_case_analysis {} unset_case_analysis
interp alias {} reset_case_analysis {} unset_case_analysis

interp alias {} remove_timing_derate {} unset_timing_derate
interp alias {} reset_timing_derate {} unset_timing_derate

interp alias {} remove_path_exceptions {} unset_path_exceptions
interp alias {} reset_path_exceptions {} unset_path_exceptions

interp alias {} remove_data_check {} unset_data_check
interp alias {} reset_data_check {} unset_data_check

interp alias {} remove_clock_transition {} unset_clock_transition
interp alias {} reset_clock_transition {} unset_clock_transition

interp alias {} remove_clock_uncertainty {} unset_clock_uncertainty
interp alias {} reset_clock_uncertainty {} unset_clock_uncertainty

interp alias {} remove_clock_latency {} unset_clock_latency
interp alias {} reset_clock_latency {} unset_clock_latency

interp alias {} remove_disable_timing {} unset_disable_timing
interp alias {} reset_disable_timing {} unset_disable_timing

interp alias {} remove_disable_timing_cell {} unset_disable_timing_cell
interp alias {} reset_disable_timing_cell {} unset_disable_timing_cell

interp alias {} remove_disable_timing_instance {} unset_disable_timing_instance
interp alias {} reset_disable_timing_instance {} unset_disable_timing_instance

# Get DB (only program_short_name supported for now)
proc get_db { attr } {
  if { $attr == "program_short_name" } {
    return "opensta"
  } else {
    error "get_db: unsupported attribute $attr"
  }
}

# Get attribute
sta::define_cmd_args "get_attribute" {args}

proc get_attribute {args} {
  sta::parse_key_args "get_attribute" args keys {} flags {-quiet}
  set quiet [info exists flags(-quiet)]
  set arg1 [lindex $args 0]
  set arg2 [lindex $args 1]

  # Suppress unknown property warning
  if { $quiet } {
    suppress_msg 9000
  }
  if { [sta::is_object $arg1] } {
    if { [sta::is_collection $arg1] } {
      set arg1 [collection_at_index $arg1 0]
    }
    set result [get_property $arg1 $arg2]
  } elseif { [sta::is_object $arg2] } {
    if { [sta::is_collection $arg2] } {
      set arg2 [collection_at_index $arg2 0]
    }
    set result [get_property $arg2 $arg1]
  } else {
    if { $quiet } {
      unsuppress_msg 9000
    }
    error "get_attribute: invalid object $arg1 or $arg2"
  }
  # Re-enable warning after the call
  if { $quiet } {
    unsuppress_msg 9000
  }
  return $result
}

################################################################
# Unsupported commands (for now)
################################################################

# Fanin/fanout commands all_fanin and all_fanout
proc all_fanin { args } {
  puts "Warning: all_fanin not supported, will return empty list"
  return [list]
}
proc all_fanout { args } {
  puts "Warning: all_fanout not supported, will return empty list"
  return [list]
}

# Set clock jitter
proc set_clock_jitter { args } {
  puts "Warning: set_clock_jitter not supported"
}

# Get liberty timing arcs
proc get_lib_timing_arcs { args } {
  puts "Warning: get_lib_timing_arcs not supported, will return empty list"
  return [list]
}

# Suppress message (ignore/to be implemented)
proc suppress_message { args } {
  puts "Warning: suppress_message not supported, command ignored"
}

################################################################
# TCL extras
################################################################

# Add echo alias
interp alias {} echo {} puts

# Add date getter
proc date {} {
  return [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S"]
}

# Add memory usage getter
proc mem {} {
  return [exec ps -o rss= -p [pid]]
}

# Set difference function
proc ldiff {a b} {
  set result {}
  foreach x $a {
    if {[lsearch -exact $b $x] == -1} {
      lappend result $x
    }
  }
  return $result
}

################################################################
# TCL 8.6 forward-compatibility
################################################################

proc try {args} {
    # Require at least one argument.
    if {![llength $args]} {
        throw {TCL WRONGARGS} "wrong # args: should be\
                \"try body ?handler ...? ?finally script?\""
    }

    # Scan arguments.
    set args [lassign $args body]
    set handlers {}
    while {[llength $args]} {
        set args [lassign $args type]
        switch $type {
        on {
            if {[llength $args] < 3} {
                throw {TCL OPERATION TRY ON ARGUMENT} "wrong # args to on\
                        clause: must be \"... on code variableList script\""
            }
            set args [lassign $args code variableList script]
            if {![string is integer -strict $code]} {
                if {[regexp {^[ \f\n\r\t\v]*[-+]?\d+[ \f\n\r\t\v]*$} $code]
                 || [set newCode [lsearch -exact\
                            {ok error return break continue} $code]] < 0} {
                    throw {TCL RESULT ILLEGAL_CODE} "bad completion code\
                            \"$code\": must be ok, error, return, break,\
                            continue, or an integer"
                }
                set code $newCode
            }
            lappend handlers on $code $variableList $script
        } trap {
            if {[llength $args] < 3} {
                throw {TCL OPERATION TRY TRAP ARGUMENT} "wrong # args to\
                        trap clause: must be \"... trap pattern\
                        variableList script\""
            }
            set args [lassign $args pattern variableList script]
            if {[catch {list {*}$pattern} pattern]} {
                throw {TCL OPERATION TRY TRAP EXNFORMAT} "bad prefix\
                        '$pattern': must be a list"
            }
            lappend handlers trap $pattern $variableList $script
        } finally {
            if {![llength $args]} {
                throw {TCL OPERATION TRY FINALLY ARGUMENT} "wrong # args\
                        to finally clause: must be \"... finally script\""
            }
            set args [lassign $args finally]
            if {[llength $args]} {
                throw {TCL OPERATION TRY FINALLY NONTERMINAL} "finally\
                        clause must be last"
            }
        } default {
            throw [list TCL LOOKUP INDEX {handler type} $type] "bad handler\
                    type \"$type\": must be finally, on, or trap"
        }}
    }
    if {[info exists script] && $script eq "-"} {
        throw {TCL OPERATION TRY BADFALLTHROUGH} "last non-finally clause must\
                not have a body of \"-\""
    }

    # Evaluate the script body and intercept errors.
    set code [catch {uplevel 1 $body} result options]

    # Search for and evaluate the first matching handler.
    foreach {type pattern varList script} $handlers {
        if {![info exists next] && ($type ne "on" || $pattern != $code)
         && ($type ne "trap" || ![dict exists $options -errorcode]
          || $pattern ne [lrange [dict get $options -errorcode]\
                0 [expr {[llength $pattern] - 1}]])} {
            # Skip this handler if it doesn't match.
        } elseif {$script eq "-"} {
            # If the script is "-", evaluate the next handler script that is not
            # "-", regardless of the match criteria.
            set next {}
        } else {
            # Evaluate the handler script and intercept errors.
            if {[catch {
                if {[llength $varList] >= 1} {
                    uplevel 1 [list set [lindex $varList 0] $result]
                }
                if {[llength $varList] >= 2} {
                    uplevel 1 [list set [lindex $varList 1] $options]
                }
                uplevel 1 $script
            } result newOptions] && [dict exists $newOptions -errorcode]} {
                dict set newOptions -during $options
            }
            set options $newOptions

            # Stop after evaluating the first matching handler script.
            break
        }
    }

    # Evaluate the finally clause and intercept errors.
    if {[info exists finally]
     && [catch {uplevel 1 $finally} newResult newOptions]} {
        if {[dict exists $newOptions -errorcode]} {
            dict set newOptions -during $options
        }
        set options $newOptions
        set result $newResult
    }

    # Return any errors generated by the handler scripts.
    dict incr options -level
    return {*}$options $result
}

proc throw {type message} {
    if {![llength $type]} {
        return -code error -errorcode {TCL OPERATION THROW BADEXCEPTION}\
                "type must be non-empty list"
    } else {
        return -code error -errorcode $type $message
    }
}
