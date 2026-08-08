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
  puts $ppa_json "  \"max_area\": [sta::max_area],"
  puts $ppa_json "  \"max_dynamic_power\": [sta::max_dynamic_power],"
  puts $ppa_json "  \"max_leakage_power\": [sta::max_leakage_power],"
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
interp alias {} unset_dont_touch {} return -level 0

# Set dont_touch_network attribute (ignore/to be implemented)
interp alias {} set_dont_touch_network {} return -level 0
interp alias {} unset_dont_touch_network {} return -level 0

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

################################################################
# Database access (get_db/set_db)
################################################################

namespace eval sta {

# Root collection names mapped to {getter is_hierarchical}. The
# is_hierarchical value splits leaf objects (insts/pins) from hierarchical ones
# (hinsts/hpins). An empty value means the object type has no hierarchy
# distinction.
array set get_db_roots {
  inst        {get_cells false}
  insts       {get_cells false}
  instance    {get_cells false}
  instances   {get_cells false}
  hinst       {get_cells true}
  hinsts      {get_cells true}
  pin         {get_pins false}
  pins        {get_pins false}
  hpin        {get_pins true}
  hpins       {get_pins true}
  net         {get_nets {}}
  nets        {get_nets {}}
  port        {get_ports {}}
  ports       {get_ports {}}
  clock       {get_clocks {}}
  clocks      {get_clocks {}}
  base_cell   {get_lib_cells {}}
  base_cells  {get_lib_cells {}}
  lib_cell    {get_lib_cells {}}
  lib_cells   {get_lib_cells {}}
  base_pin    {get_lib_pins {}}
  base_pins   {get_lib_pins {}}
  lib_pin     {get_lib_pins {}}
  lib_pins    {get_lib_pins {}}
  lib         {get_libs {}}
  libs        {get_libs {}}
  library     {get_libs {}}
  libraries   {get_libs {}}
}

# object_type name mapped to the define_property -object_type name.
array set get_db_prop_types {
  Instance       instance
  Pin            pin
  Net            net
  Clock          clock
  Port           port
  Cell           cell
  Library        library
  LibertyCell    liberty_cell
  LibertyPort    liberty_port
  LibertyLibrary liberty_library
}

# object_type name mapped to the .obj_type attribute value.
array set get_db_obj_types {
  Instance       inst
  Pin            pin
  Net            net
  Clock          clock
  Port           port
  Cell           cell
  Library        library
  LibertyCell    base_cell
  LibertyPort    base_pin
  LibertyLibrary lib
}

# Global scalars read with "get_db name" and written with "set_db name value",
# so a tool embedding sta can report its own identity.
array set get_db_globals {
  program_short_name opensta
  program_name       OpenSTA
}
set get_db_globals(program_version) [version]

# {object_type,property} pairs already registered with define_property.
array set get_db_user_props {}

# Flatten a collection, an object or a list of objects into a Tcl list.
proc get_db_to_list { objects } {
  if { $objects == "" || $objects == "NULL" } {
    return {}
  }
  set result {}
  foreach_in_collection obj $objects {
    lappend result $obj
  }
  return $result
}

# Non-empty list of objects, or {} if the argument is a pattern/attribute.
proc get_db_object_list { arg } {
  if { [is_object $arg] } {
    return [get_db_to_list $arg]
  }
  if { [catch { llength $arg } length] || $length == 0 } {
    return {}
  }
  foreach element $arg {
    if { ![is_object $element] } {
      return {}
    }
  }
  return $arg
}

# Split a pattern at its last hierarchy separator.
proc get_db_split_path { pattern } {
  global hierarchy_separator

  set index [string last $hierarchy_separator $pattern]
  if { $index < 0 } {
    return [list "" $pattern]
  }
  return [list [string range $pattern 0 [expr { $index - 1 }]] \
            [string range $pattern [expr { $index + 1 }] end]]
}

# Match one name component the way PatternMatch does, anchoring regexps.
proc get_db_name_match { pattern name regexp nocase } {
  if { $regexp } {
    if { $nocase } {
      return [regexp -nocase -- "^(?:$pattern)$" $name]
    }
    return [regexp -- "^(?:$pattern)$" $name]
  }
  if { $nocase } {
    return [string match -nocase $pattern $name]
  }
  return [string match $pattern $name]
}

# A pin pattern matches against the whole hierarchical path, while the sdc
# network walks the pattern one hierarchy level per separator. Splitting at the
# last separator and resolving the instance part with -hierarchical (which does
# match whole path names) gives whole path matching without scanning every pin
# in the design.
proc get_db_find_pins { pattern want_hier opts regexp nocase } {
  lassign [get_db_split_path $pattern] inst_pattern port_pattern
  if { $inst_pattern == "" } {
    return [get_db_to_list [get_pins {*}$opts \
                              -filter "is_hierarchical==$want_hier" $pattern]]
  }
  set pins {}
  foreach inst [get_db_to_list [get_cells {*}$opts -hierarchical $inst_pattern]] {
    foreach pin [get_db_inst_pins $inst] {
      if { [$pin is_hierarchical] == $want_hier \
             && [get_db_name_match $port_pattern [$pin port_name] \
                   $regexp $nocase] } {
        lappend pins $pin
      }
    }
  }
  return $pins
}

proc get_db_find_nets { pattern opts regexp nocase } {
  lassign [get_db_split_path $pattern] inst_pattern net_pattern
  if { $inst_pattern == "" } {
    return [get_db_to_list [get_nets {*}$opts $pattern]]
  }
  set nets {}
  foreach inst [get_db_to_list [get_cells {*}$opts -hierarchical $inst_pattern]] {
    set net_iter [$inst net_iterator]
    while { [$net_iter has_next] } {
      set net [$net_iter next]
      if { [get_db_name_match $net_pattern [get_name $net] $regexp $nocase] } {
        lappend nets $net
      }
    }
    $net_iter finish
  }
  return $nets
}

proc get_db_query { root pattern regexp nocase quiet } {
  variable get_db_roots

  lassign $get_db_roots($root) getter hierarchical
  # The native no-match warning does not fire on every path taken below, so
  # get_db reports its own instead.
  set opts {-quiet}
  if { $regexp } {
    lappend opts -regexp
  }
  if { $nocase } {
    lappend opts -nocase
  }

  if { $getter == "get_pins" } {
    set objects [get_db_find_pins $pattern [string equal $hierarchical "true"] \
                   $opts $regexp $nocase]
  } elseif { $getter == "get_nets" } {
    set objects [get_db_find_nets $pattern $opts $regexp $nocase]
  } elseif { $getter == "get_cells" } {
    set objects [get_db_to_list [get_cells {*}$opts -hierarchical \
                                   -filter "is_hierarchical==$hierarchical" \
                                   $pattern]]
  } else {
    set objects [get_db_to_list [$getter {*}$opts $pattern]]
  }
  if { !$quiet && [llength $objects] == 0 } {
    sta_warn 2233 "get_db $root '$pattern' not found."
  }
  return $objects
}

proc get_db_object_or_empty { obj } {
  if { $obj == "" || $obj == "NULL" || ![is_object $obj] } {
    return {}
  }
  return [list $obj]
}

# Power and ground pins are dropped, matching the native get_pins.
proc get_db_inst_pins { inst } {
  set pins {}
  set pin_iter [$inst pin_iterator]
  while { [$pin_iter has_next] } {
    set pin [$pin_iter next]
    if { ![$pin is_pwr_gnd] } {
      lappend pins $pin
    }
  }
  $pin_iter finish
  return $pins
}

proc get_db_inst_children { inst } {
  set insts {}
  set child_iter [$inst child_iterator]
  while { [$child_iter has_next] } {
    lappend insts [$child_iter next]
  }
  $child_iter finish
  return $insts
}

# Pins on a net, optionally restricted to those answering true to predicate.
proc get_db_net_pins { net { predicate "" } } {
  set pins {}
  set pin_iter [$net pin_iterator]
  while { [$pin_iter has_next] } {
    set pin [$pin_iter next]
    if { ![$pin is_pwr_gnd] && ($predicate == "" || [$pin $predicate]) } {
      lappend pins $pin
    }
  }
  $pin_iter finish
  return $pins
}

# Unknown properties warn with msg 9000 and return an empty value, so -quiet
# has to suppress that message rather than test for it.
proc get_db_property { obj attr quiet } {
  if { $quiet } {
    suppress_msg 9000
  }
  set failed [catch { get_property $obj $attr } value]
  if { $quiet } {
    unsuppress_msg 9000
  }
  if { $failed } {
    return {}
  }
  return $value
}

# One attribute of one object, always as a list so chains can be flattened.
proc get_db_attr { obj attr quiet } {
  variable get_db_obj_types

  if { ![is_object $obj] } {
    sta_warn 2223 "get_db attribute .$attr requested on non-object '$obj'."
    return {}
  }
  set type [object_type $obj]

  # .name is the hierarchical name and .base_name is the leaf name, which is
  # the reverse of the OpenSTA property names.
  if { $attr == "name" } {
    set attr "full_name"
  } elseif { $attr == "base_name" } {
    set attr "name"
  } elseif { $attr == "obj_type" } {
    if { [info exists get_db_obj_types($type)] } {
      return [list $get_db_obj_types($type)]
    }
    return [list $type]
  }

  if { $type == "Pin" } {
    if { $attr == "inst" || $attr == "instance" } {
      return [get_db_object_or_empty [$obj instance]]
    } elseif { $attr == "net" } {
      return [get_db_object_or_empty [$obj net]]
    } elseif { $attr == "port" } {
      return [get_db_object_or_empty [$obj port]]
    } elseif { $attr == "base_pin" || $attr == "lib_pin" } {
      return [get_db_object_or_empty [$obj liberty_port]]
    }
  } elseif { $type == "Instance" } {
    if { $attr == "pins" } {
      return [get_db_inst_pins $obj]
    } elseif { $attr == "insts" || $attr == "instances" \
                 || $attr == "children" } {
      return [get_db_inst_children $obj]
    } elseif { $attr == "parent" || $attr == "hinst" } {
      return [get_db_object_or_empty [$obj parent]]
    } elseif { $attr == "base_cell" || $attr == "lib_cell" } {
      return [get_db_object_or_empty [$obj liberty_cell]]
    }
  } elseif { $type == "Net" } {
    if { $attr == "pins" } {
      return [get_db_net_pins $obj]
    } elseif { $attr == "drivers" } {
      return [get_db_net_pins $obj is_driver]
    } elseif { $attr == "loads" } {
      return [get_db_net_pins $obj is_load]
    } elseif { $attr == "inst" || $attr == "instance" } {
      return [get_db_object_or_empty [$obj instance]]
    }
  }
  return [get_db_property $obj $attr $quiet]
}

# Walk a dotted attribute chain such as .pins.net over a list of objects,
# flattening the results of each step.
proc get_db_attrs { objects attr_path quiet } {
  set names [split [string range $attr_path 1 end] "."]
  if { $names == {} } {
    # A lone "." splits to nothing, so let the loop below report it.
    set names [list ""]
  }
  set current $objects
  foreach attr $names {
    if { $attr == "" } {
      sta_error 2224 "get_db attribute '$attr_path' has an empty component."
    }
    set next {}
    foreach obj $current {
      foreach value [get_db_attr $obj $attr $quiet] {
        lappend next $value
      }
    }
    set current $next
  }
  return $current
}

################################################################
# -if expression support.
#
# The native -filter expressions only understand single level property names,
# so get_db needs its own evaluator for chains like .inst.is_memory.
################################################################

proc get_db_is_delimiter { ch } {
  return [expr { [string is space -strict $ch]
                 || [string first $ch "()&|!=<>~"] >= 0 }]
}

proc get_db_lex { expression } {
  set tokens {}
  set length [string length $expression]
  set i 0
  while { $i < $length } {
    set ch [string index $expression $i]
    set two [string range $expression $i [expr { $i + 1 }]]
    if { [string is space -strict $ch] } {
      incr i
    } elseif { $ch == "(" } {
      lappend tokens [list lparen $ch]
      incr i
    } elseif { $ch == ")" } {
      lappend tokens [list rparen $ch]
      incr i
    } elseif { $two == "&&" || $two == "||" } {
      lappend tokens [list [expr { $two == "&&" ? "and" : "or" }] $two]
      incr i 2
    } elseif { [lsearch -exact {== != >= <= =~ !~} $two] >= 0 } {
      lappend tokens [list op $two]
      incr i 2
    } elseif { $ch == ">" || $ch == "<" } {
      lappend tokens [list op $ch]
      incr i
    } elseif { $ch == "!" } {
      lappend tokens [list not $ch]
      incr i
    } elseif { $ch == "&" || $ch == "|" } {
      lappend tokens [list [expr { $ch == "&" ? "and" : "or" }] $ch]
      incr i
    } elseif { $ch == "." } {
      set j [expr { $i + 1 }]
      while { $j < $length \
                && [string match {[A-Za-z0-9_.]} [string index $expression $j]] } {
        incr j
      }
      lappend tokens [list attr [string range $expression $i [expr { $j - 1 }]]]
      set i $j
    } elseif { $ch == "\"" } {
      set j [string first "\"" $expression [expr { $i + 1 }]]
      if { $j < 0 } {
        sta_error 2225 "get_db -if expression has an unterminated string."
      }
      lappend tokens [list lit [string range $expression [expr { $i + 1 }] \
                                  [expr { $j - 1 }]]]
      set i [expr { $j + 1 }]
    } else {
      set j $i
      while { $j < $length \
                && ![get_db_is_delimiter [string index $expression $j]] } {
        incr j
      }
      lappend tokens [list lit [string range $expression $i [expr { $j - 1 }]]]
      set i $j
    }
  }
  return $tokens
}

proc get_db_peek { tokens i } {
  if { $i < [llength $tokens] } {
    return [lindex $tokens $i 0]
  }
  return ""
}

proc get_db_parse_or { tokens i_var } {
  upvar 1 $i_var i

  set node [get_db_parse_and $tokens i]
  while { [get_db_peek $tokens $i] == "or" } {
    incr i
    set node [list or $node [get_db_parse_and $tokens i]]
  }
  return $node
}

proc get_db_parse_and { tokens i_var } {
  upvar 1 $i_var i

  set node [get_db_parse_unary $tokens i]
  while { [get_db_peek $tokens $i] == "and" } {
    incr i
    set node [list and $node [get_db_parse_unary $tokens i]]
  }
  return $node
}

proc get_db_parse_unary { tokens i_var } {
  upvar 1 $i_var i

  set kind [get_db_peek $tokens $i]
  if { $kind == "not" } {
    incr i
    return [list not [get_db_parse_unary $tokens i]]
  }
  if { $kind == "lparen" } {
    incr i
    set node [get_db_parse_or $tokens i]
    if { [get_db_peek $tokens $i] != "rparen" } {
      sta_error 2226 "get_db -if expression is missing a ')'."
    }
    incr i
    return $node
  }
  return [get_db_parse_compare $tokens i]
}

proc get_db_parse_compare { tokens i_var } {
  upvar 1 $i_var i

  set lhs [get_db_parse_operand $tokens i]
  if { [get_db_peek $tokens $i] == "op" } {
    set op [lindex $tokens $i 1]
    incr i
    return [list compare $op $lhs [get_db_parse_operand $tokens i]]
  }
  return [list truthy $lhs]
}

proc get_db_parse_operand { tokens i_var } {
  upvar 1 $i_var i

  set kind [get_db_peek $tokens $i]
  if { $kind == "attr" || $kind == "lit" } {
    set operand [list $kind [lindex $tokens $i 1]]
    incr i
    return $operand
  }
  sta_error 2227 "get_db -if expression expects an attribute or value."
}

proc get_db_normalize_bool { value } {
  if { [string equal -nocase $value "true"] } {
    return 1
  }
  if { [string equal -nocase $value "false"] } {
    return 0
  }
  return $value
}

proc get_db_truthy { value } {
  set value [get_db_normalize_bool $value]
  if { $value == "" } {
    return 0
  }
  if { [string is double -strict $value] } {
    return [expr { $value != 0 }]
  }
  return 1
}

proc get_db_equal { lhs rhs } {
  if { [string is double -strict $lhs] && [string is double -strict $rhs] } {
    return [expr { $lhs == $rhs }]
  }
  return [string equal $lhs $rhs]
}

proc get_db_compare { op lhs rhs } {
  set lhs [get_db_normalize_bool $lhs]
  set rhs [get_db_normalize_bool $rhs]
  if { $op == "=~" } {
    return [string match $rhs $lhs]
  } elseif { $op == "!~" } {
    return [expr { ![string match $rhs $lhs] }]
  } elseif { $op == "==" } {
    if { [string match {*[*?]*} $rhs] } {
      return [string match $rhs $lhs]
    }
    return [get_db_equal $lhs $rhs]
  } elseif { $op == "!=" } {
    if { [string match {*[*?]*} $rhs] } {
      return [expr { ![string match $rhs $lhs] }]
    }
    return [expr { ![get_db_equal $lhs $rhs] }]
  } elseif { $op == ">" } {
    return [expr { $lhs > $rhs }]
  } elseif { $op == "<" } {
    return [expr { $lhs < $rhs }]
  } elseif { $op == ">=" } {
    return [expr { $lhs >= $rhs }]
  }
  return [expr { $lhs <= $rhs }]
}

proc get_db_operand_value { operand obj quiet } {
  if { [lindex $operand 0] == "attr" } {
    set values [get_db_attrs [list $obj] [lindex $operand 1] $quiet]
    if { [llength $values] == 0 } {
      return ""
    }
    return [lindex $values 0]
  }
  return [lindex $operand 1]
}

proc get_db_eval { node obj quiet } {
  set kind [lindex $node 0]
  if { $kind == "or" } {
    return [expr { [get_db_eval [lindex $node 1] $obj $quiet]
                   || [get_db_eval [lindex $node 2] $obj $quiet] }]
  } elseif { $kind == "and" } {
    return [expr { [get_db_eval [lindex $node 1] $obj $quiet]
                   && [get_db_eval [lindex $node 2] $obj $quiet] }]
  } elseif { $kind == "not" } {
    return [expr { ![get_db_eval [lindex $node 1] $obj $quiet] }]
  } elseif { $kind == "compare" } {
    return [get_db_compare [lindex $node 1] \
              [get_db_operand_value [lindex $node 2] $obj $quiet] \
              [get_db_operand_value [lindex $node 3] $obj $quiet]]
  }
  return [get_db_truthy [get_db_operand_value [lindex $node 1] $obj $quiet]]
}

proc get_db_filter { objects expression quiet } {
  set tokens [get_db_lex $expression]
  if { [llength $tokens] == 0 } {
    return $objects
  }
  set i 0
  set tree [get_db_parse_or $tokens i]
  if { $i != [llength $tokens] } {
    sta_error 2228 "get_db -if expression has trailing garbage."
  }
  set result {}
  foreach obj $objects {
    if { [get_db_eval $tree $obj $quiet] } {
      lappend result $obj
    }
  }
  return $result
}

# Duplicate removal that preserves order.
proc get_db_unique { values } {
  set result {}
  array set seen {}
  foreach value $values {
    if { ![info exists seen($value)] } {
      set seen($value) 1
      lappend result $value
    }
  }
  return $result
}

# Record an attribute value so get_db can read it back.
proc set_db_user_property { objects attr value } {
  variable get_db_prop_types
  variable get_db_user_props

  foreach obj $objects {
    set type [object_type $obj]
    if { [info exists get_db_prop_types($type)] } {
      set prop_type $get_db_prop_types($type)
      if { ![info exists get_db_user_props($prop_type,$attr)] } {
        define_property -object_type $prop_type -type string $attr
        set get_db_user_props($prop_type,$attr) 1
      }
      set_property $obj $attr $value
    } else {
      sta_warn 2229 "set_db cannot set .$attr on $type objects."
    }
  }
}

# sta namespace end.
}

sta::define_cmd_args "get_db" \
  {[-if expr] [-unique] [-quiet] [-regexp] [-nocase]\
     objects|collection_name [pattern] [.attribute]}

proc get_db { args } {
  sta::parse_key_args "get_db" args keys {-if} \
    flags {-unique -quiet -regexp -nocase}

  if { [llength $args] == 0 } {
    sta::sta_error 2220 "get_db requires an object collection or attribute name."
  }
  set quiet [info exists flags(-quiet)]
  set first [lindex $args 0]
  set rest [lrange $args 1 end]

  set objects [sta::get_db_object_list $first]
  if { $objects == {} && ![sta::is_object $first] } {
    # Not an object argument, so it names a root collection or a global.
    if { [string trim $first] == "" } {
      return {}
    }
    if { [string index $first 0] == "." } {
      sta::sta_error 2221 "get_db attribute '$first' has no object collection."
    }
    if { [info exists sta::get_db_globals($first)] } {
      if { [llength $rest] != 0 } {
        sta::cmd_usage_error "get_db"
      }
      return $sta::get_db_globals($first)
    }
    if { ![info exists sta::get_db_roots($first)] } {
      sta::sta_error 2222 "get_db '$first' is not a supported collection or attribute."
    }
    set pattern "*"
    if { [llength $rest] > 0 && [string index [lindex $rest 0] 0] != "." } {
      set pattern [lindex $rest 0]
      set rest [lrange $rest 1 end]
    }
    set objects [sta::get_db_query $first $pattern \
                   [info exists flags(-regexp)] [info exists flags(-nocase)] \
                   $quiet]
  }

  set attr_path ""
  if { [llength $rest] > 0 } {
    set attr_path [lindex $rest 0]
    set rest [lrange $rest 1 end]
    if { [string index $attr_path 0] != "." } {
      sta::sta_error 2230 "get_db attribute '$attr_path' must start with '.'."
    }
  }
  if { [llength $rest] != 0 } {
    sta::cmd_usage_error "get_db"
  }

  if { [info exists keys(-if)] } {
    set objects [sta::get_db_filter $objects $keys(-if) $quiet]
  }
  set result $objects
  if { $attr_path != "" } {
    set result [sta::get_db_attrs $objects $attr_path $quiet]
  }
  if { [info exists flags(-unique)] } {
    set result [sta::get_db_unique $result]
  }
  # Return a bare value for a single result so that names containing brackets
  # are not brace quoted by the list representation.
  if { [llength $result] == 1 } {
    return [lindex $result 0]
  }
  return $result
}

sta::define_cmd_args "set_db" {objects .attribute value|global_name value}

proc set_db { args } {
  if { [llength $args] == 2 && [string index [lindex $args 0] 0] != "." } {
    lassign $args name value
    if { [info exists sta::get_db_globals($name)] } {
      set sta::get_db_globals($name) $value
      return
    }
    sta::sta_warn 2231 "set_db $name is not supported, command ignored."
    return
  }
  if { [llength $args] != 3 } {
    sta::cmd_usage_error "set_db"
  }
  lassign $args objects attr value

  if { [string index $attr 0] != "." } {
    sta::sta_error 2232 "set_db attribute '$attr' must start with '.'."
  }
  set attr [string range $attr 1 end]
  set objects [sta::get_db_object_list $objects]
  if { $objects == {} } {
    return
  }

  if { $attr == "dont_touch" || $attr == "dont_touch_network" } {
    if { [sta::get_db_truthy $value] } {
      set_$attr $objects
    } else {
      unset_$attr $objects
    }
  }
  sta::set_db_user_property $objects $attr $value
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

# Fanin/fanout commands all_fanin and all_fanout. get_fanin/get_fanout accept a
# superset of the sdc flags, but stop at hierarchy crossings unless -flat is
# given, while sdc defines the traversal over the flattened netlist.
interp alias {} all_fanin {} get_fanin -flat
interp alias {} all_fanout {} get_fanout -flat

################################################################
# Unsupported commands (for now)
################################################################

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
