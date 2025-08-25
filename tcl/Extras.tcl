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

}

################################################################
# Miscellaneous commands
################################################################

sta::define_cmd_args "set_dont_use" {lib_cell_name_pattern}
     
proc set_dont_use {lib_cell_name_pattern} {
  set targets [get_lib_cells -filter "name=~$lib_cell_name_pattern"]
  foreach target $targets {
    $target set_dont_use
  }
}

sta::define_cmd_args "unset_dont_use" {lib_cell_name_pattern}
     
proc unset_dont_use {lib_cell_name_pattern} {
  set targets [get_lib_cells -filter "name=~$lib_cell_name_pattern"]
  foreach target $targets {
    $target unset_dont_use
  }
}

# Set dont_touch attribute (ignore/to be implemented)
interp alias {} set_dont_touch {} return -level 0

# Set dont_touch_network attribute (ignore/to be implemented)
interp alias {} set_dont_touch_network {} return -level 0

# Get object name
interp alias {} get_object_name {} get_full_name

# Query objects (ignore/to be implemented)
interp alias {} query_objects {} return -level 0

# Get DB (only program_short_name supported for now)
proc get_db { attr } {
  if { $attr == "program_short_name" } {
    return "opensta"
  } else {
    error "get_db: unsupported attribute $attr"
  }
}

# Get attribute
sta::define_cmd_args "get_attribute" {arg1 arg2}

proc get_attribute {arg1 arg2} {
  if { [sta::is_object $arg1] } {
    return [get_property $arg1 $arg2]
  } elseif { [sta::is_object $arg2] } {
    return [get_property $arg2 $arg1]
  } else {
    error "get_attribute: invalid object $arg1 or $arg2"
  }
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
