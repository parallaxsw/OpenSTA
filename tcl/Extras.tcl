################################################################
# Miscellaneous commands
################################################################

# Set dont_use attribute (ignore)
interp alias {} set_dont_use {} return -level 0

# Set dont_touch attribute (ignore)
interp alias {} set_dont_touch {} return -level 0

# Get object name
interp alias {} get_object_name {} return -level 0

# Query objects
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
interp alias {} get_attribute {} get_property

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
