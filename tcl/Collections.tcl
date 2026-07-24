################################################################
#
# Collection commands from Altera Quartus
#
# This script applies collection commands to both simple Tcl lists
# and Sequences marked with COLLECTION_HELPERS in .i files.
#
################################################################

# Add objects to a collection, resulting in a new collection. The base
# collection remains unchanged. The return type depends on whether the
# collection is a Tcl list or otherwise.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/add_to_collection-quartus-sta
proc add_to_collection {collection objects} {
  if {[sta::is_collection $collection]} {
    return [sta::collection_plus $collection $objects]
  } else {
    if {[sta::is_collection $objects]} {
      foreach_in_collection element $objects {
        lappend collection $element
      }
      return $collection
    } else {
      return [concat $collection $objects]
    }
  }
}


# Duplicates the contents of a collection, resulting in a new collection. The base collection remains unchanged.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/dni-copy_collection-quartus-dcmd_dni
interp alias {} copy_collection {} index_collection  

# The foreach_in_collection command is similar to the foreach Tcl command. Use it to iterate through all elements in a collection.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/foreach_in_collection-quartus-misc
proc foreach_in_collection {variable_name collection body} {
  if {[sta::is_collection $collection]} {
    set it [sta::collection_get_iterator $collection]
    set rc 0
    while {[$it has_next]} {
      set current [$it next]
      uplevel 1 [list set $variable_name $current]
      set rc [catch {uplevel 1 $body} result options]
      if {$rc == 1} {
        # error - finish iterator, then re-throw
        $it finish
        return -options $options $result
      } elseif {$rc == 3} {
        # break
        break
      } elseif {$rc == 4} {
        # continue - do nothing, loop continues
      } elseif {$rc != 0} {
        $it finish
        return -options $options $result
      }
    }
    $it finish
  } else {
    foreach current $collection {
      uplevel 1 [list set $variable_name $current]
      uplevel 1 $body
    }
  }
}

# Use the get_collection_size command to get the number of elements in a collection.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/get_collection_size-quartus-misc
proc get_collection_size {collection} {
  if {[sta::is_collection $collection]} {
    return [sta::collection_count $collection]
  } else {
    return [llength $collection]
  }
}

# Given a collection and an index, if the index is in range, create a new collection containing only the single object.
# Optionally a second index can be passed to create a new collection with the objects between the two indices in the base collection (inclusive).
# As a custom extension to the spec, passing neither index simply creates a copy.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/dni-index_collection-quartus-dcmd_dni
proc index_collection {collection {index1 ""} {index2 ""}} {
  if { "$index2" == "" } {
    if { "$index1" == "" } {
      set index1 "0"
      set index2 "end"
    } else {
      set index2 "$index1"
    }
  }
  if {[sta::is_collection $collection]} {
    return [sta::collection_slice $collection $index1 $index2]
  } else {
    return [lrange $collection $index1 $index2]
  }
}

proc collection_at_index {collection index} {
  if {[sta::is_collection $collection]} {
    return [sta::collection_element_at $collection $index]
  } else {
    return [lindex $collection $index]
  }
}

# Returns the number of objects in a collection.
interp alias {} sizeof_collection {} get_collection_size

# Sorts a collection based on one or more attributes, resulting in a new,
# sorted collection. The sort is ascending by default.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/dni-sort_collection-quartus-dcmd_dni
proc sort_collection { args } {
  sta::parse_key_args "sort_collection" args \
    keys {-limit} \
    flags {-ascending -descending -dictionary -real}

  sta::check_argc_eq2 "sort_collection" $args

  set collection [lindex $args 0]
  set criteria [lindex $args 1]

  if { [info exists flags(-real)] && [info exists flags(-dictionary)]} {
    sta::sta_error 150 "sort_collection -real and -dictionary are mutually exclusive"
  }
  if { [info exists flags(-ascending)] && [info exists flags(-descending)] } {
    sta::sta_error 151 "sort_collection -ascending and -descending are mutually exclusive"
  }

  set limit "end"
  if { [info exists keys(-limit)] } {
    set limit $keys(-limit)
  }

  set list_format_arg [list]
  if { [sta::is_collection $collection] } {
    lappend list_format_arg -list_format
  }

  set result [sta::collection_sorted $collection $criteria [info exists flags(-descending)] [expr ![info exists flags(-dictionary)]]]

  return [query_collection $result -limit $limit {*}$list_format_arg]
}

# Returns a part of the collection.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/query_collection-quartus-sta
proc query_collection { args } {
  sta::parse_key_args "query_collection" args \
    keys {-limit} \
    flags {-all -list_format -report_format}

  sta::check_argc_eq1 "query_collection" $args

  set collection [lindex $args 0]
  set limit 20

  if { [info exists keys(-limit)] } {
    set limit $keys(-limit)
  }

  if { [info exists flags(-all)] } {
    set limit "end"
  }

  if { [info exists flags(-report_format)] } {
    sta::sta_warn 152 "query_collection flag -report_format is currently unsupported and will be ignored."
  }

  set result [index_collection $collection 0 $limit]

  if { [info exists flags(-list_format)] } {
    set result_list ""
    foreach_in_collection element $result {
      lappend result_list $element
    }
    return $result_list
  }

  return $result
}

# Append objects to a collection
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/dni-append_to_collection-quartus-dcmd_dni
proc append_to_collection { args } {
  sta::parse_key_args "append_to_collection" args \
    keys {} \
    flags {-unique}

  sta::check_argc_eq2 "append_to_collection" $args

  set collection [lindex $args 0]
  set objects [lindex $args 1]

  upvar 1 $collection coll

  # If the target variable does not exist yet, auto-initialize it
  if { ![info exists coll] } {
    set coll {}
  }

  if { [sta::is_collection $coll] } {
    sta::collection_append_inplace $coll $objects [info exists flags(-unique)]
  } else {
    # tcl list cannot be modified in-place, use collection_plus
    set coll [sta::collection_plus $coll $objects [info exists flags(-unique)]]
  }
}

# Remove objects from a collection, resulting in a new collection.
# The base collection remains unchanged.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/remove_from_collection-quartus-sta
proc remove_from_collection { args } {
  sta::parse_key_args "remove_from_collection" args \
    keys {} \
    flags {-intersect}

  sta::check_argc_eq2 "remove_from_collection" $args

  set collection [lindex $args 0]
  set objects [lindex $args 1]
  set intersect [info exists flags(-intersect)]

  if {[sta::is_collection $collection]} {
    return [sta::collection_minus $collection $objects $intersect]
  } else {
    set result {}
    foreach item $collection {
      if { $intersect != ([lsearch -exact $objects $item] == -1)} {
        lappend result $item
      }
    }
    return $result
  }
}

# Filters an existing collection, resulting in a new collection.
# The base collection remains unchanged.
# https://docs.altera.com/r/docs/683432/25.3.1/quartus-prime-pro-edition-user-guide-scripting/dni-filter_collection-quartus-dcmd_dni
proc filter_collection { args } {
  sta::parse_key_args "filter_collection" args \
    keys {} \
    flags {-nocase -regexp -quiet}
  # SILIMATE: -quiet is silently ignored for reasons currently unclear

  sta::check_argc_eq2 "filter_collection" $args

  if { [info exists flags(-nocase)] || [info exists flags(-regexp)] } {
    sta::sta_warn 153 "filter_collection flags -nocase and -regexp are currently unsupported and will be ignored."
  }

  set collection [lindex $args 0]
  set filter [lindex $args 1]
  
  if { [sizeof_collection $collection] == 0 } {
    return $collection
  } else {
    set object_type ""
    foreach_in_collection item $collection {
      set object_type [sta::object_type $item]
      break
    }
    if { $object_type == "Pin" } {
      return [sta::filter_objs $filter $collection filter_pins "pin"]
    } elseif { $object_type == "Instance" } {
      return [sta::filter_objs $filter $collection filter_insts "instance"]
    } elseif { $object_type == "Net" } {
      return [sta::filter_objs $filter $collection filter_nets "net"]
    } elseif { $object_type == "Port" } {
      return [sta::filter_objs $filter $collection filter_ports "port"]
    } elseif { $object_type == "Edge" } {
      return [sta::filter_objs $filter $collection filter_edges "edge"]
    } elseif { $object_type == "Clock" } {
      return [sta::filter_objs $filter $collection filter_clocks "clock"]
    } elseif { $object_type == "LibertyCell" } {
      return [sta::filter_objs $filter $collection filter_lib_cells "liberty cell"]
    } elseif { $object_type == "LibertyPort" } {
      return [sta::filter_objs $filter $collection filter_lib_pins "liberty port"]
    } elseif { $object_type == "LibertyLibrary" } {
      return [sta::filter_objs $filter $collection filter_liberty_libraries "liberty library"]
    } elseif { $object_type == "TimingArcSet" } {
      return [sta::filter_objs $filter $collection filter_timing_arcs "timing arc"]
    } else {
      sta::sta_error 154 "unsupported object type $object_type."
    }
  }
}
