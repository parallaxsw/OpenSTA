################################################################
# Extra TCL commands that exist in commercial tools
################################################################

namespace eval sta {

################################################################
# Miscellaneous commands that exist in commercial tools
################################################################

# Set dont_use attribute (ignore)
define_cmd_args "set_dont_use" {object_list}
proc set_dont_use { args } { }

# Set dont_touch attribute (ignore)
define_cmd_args "set_dont_touch" {object_list}
proc set_dont_touch { args } { }

# Get DB (only program_short_name supported for now)
define_cmd_args "get_db" {attribute}

proc get_db { args } {
  parse_key_args "get_db" args keys {} flags {}
  check_argc_eq1 "get_db" $args
  set attribute [lindex $args 0]
  if { $attribute == "program_short_name" } {
    return "opensta"
  } else {
    error "get_db: unsupported attribute $attribute"
  }
}

# Get object name
interp alias {} get_object_name {} get_name

# Query objects
interp alias {} query_objects {} return -level 0

################################################################
# Unsupported commands (for now)
################################################################

# Fanin/fanout commands all_fanin and all_fanout
define_cmd_args "all_fanin" \
  {-to sink_list [-flat] [-only_cells] [-start]\
     [-levels level_count] [-pin_levels pin_count]\
     [-trace_arcs timing|enabled|all]}
proc all_fanin { args } {
  sta_warn 991 "all_fanin not supported, will return empty list"
  return [list]
}
define_cmd_args "all_fanout" \
  {-from source_list [-flat] [-only_cells] [-end]\
     [-levels level_count] [-pin_levels pin_count]\
     [-trace_arcs timing|enabled|all]}
proc all_fanout { args } {
  sta_warn 990 "all_fanout not supported, will return empty list"
  return [list]
}
    
# define_cmd_alias "all_fanin" "get_fanin"
# define_cmd_alias "all_fanout" "get_fanout"

}

################################################################
# TCL extras
################################################################

# Add echo alias
interp alias {} echo {} puts
