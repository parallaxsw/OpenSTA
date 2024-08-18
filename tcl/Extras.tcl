################################################################
# Miscellaneous commands that exist in commercial tools
################################################################

# Set dont_use attribute (ignore)
interp alias {} set_dont_use {} return -level 0

# Set dont_touch attribute (ignore)
interp alias {} set_dont_touch {} return -level 0

# Get object name
interp alias {} get_object_name {} get_name

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

################################################################
# TCL extras
################################################################

# Add echo alias
interp alias {} echo {} puts
