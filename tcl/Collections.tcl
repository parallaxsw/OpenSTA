################################################################
#
# Collection commands from Altera Quartus
#
# This script applies collection commands to both simple Tcl lists
# and Sequences marked with COLLECTION_HELPERS in .i files.
#
################################################################

# Command help is registered here; the procs below are global (not in sta::).
sta::define_cmd_args "add_to_collection" {collection objects} \
  -help {The `add_to_collection` command returns a new collection with `objects` added to `collection`. The original collection is not modified.} \
  -arg_help {
    collection {A collection or Tcl list.}
    objects {Objects or a collection to add.}
  }

sta::define_cmd_args "append_to_collection" {[-unique] collection_name objects} \
  -help {The `append_to_collection` command appends `objects` to the collection stored in the variable named `collection_name`. The variable is created if it does not exist. `-unique` skips objects already in the collection.} \
  -arg_help {
    -unique {Do not append objects that are already in the collection.}
  }

sta::define_cmd_args "copy_collection" {collection [index1] [index2]} \
  -help {The `copy_collection` command returns a copy of `collection`. It is an alias for `index_collection`; optional indices select a slice as in `index_collection`.} \
  -arg_help {
    collection {A collection or Tcl list.}
    index1 {Start index.}
    index2 {End index (inclusive).}
  }

sta::define_cmd_args "filter_collection" {[-nocase] [-regexp] [-quiet] collection filter} \
  -help {The `filter_collection` command returns the objects in `collection` that match `filter`. The original collection is not modified. `-nocase` and `-regexp` are currently ignored.} \
  -arg_help {
    -nocase {Currently ignored.}
    -regexp {Currently ignored.}
    -quiet {Currently ignored.}
    collection {A collection or Tcl list to filter.}
    filter {A filter expression. See the section "Filter Expressions".}
  }

sta::define_cmd_args "foreach_in_collection" {variable_name collection body} \
  -help {The `foreach_in_collection` command iterates over each object in `collection`, assigning it to `variable_name` and evaluating `body`. `break` and `continue` work as in `foreach`.} \
  -arg_help {
    variable_name {The loop variable name.}
    collection {A collection or Tcl list.}
    body {Tcl script evaluated for each object.}
  }

sta::define_cmd_args "get_collection_size" {collection} \
  -help {The `get_collection_size` command returns the number of objects in `collection`.} \
  -arg_help {
    collection {A collection or Tcl list.}
  }

sta::define_cmd_args "sizeof_collection" {collection} \
  -help {The `sizeof_collection` command returns the number of objects in `collection`. It is an alias for `get_collection_size`.} \
  -arg_help {
    collection {A collection or Tcl list.}
  }

sta::define_cmd_args "index_collection" {collection [index1] [index2]} \
  -help {The `index_collection` command returns a slice of `collection`. With one index, it returns the object at that index. With two indices, it returns the inclusive range. With no indices, it returns a copy of the collection.} \
  -arg_help {
    collection {A collection or Tcl list.}
    index1 {Start index (default 0 when omitted with `index2`).}
    index2 {End index (inclusive).}
  }

sta::define_cmd_args "query_collection" {[-limit count] [-all] [-list_format] [-report_format] collection} \
  -help {The `query_collection` command returns a prefix of `collection`. By default the first 20 objects are returned. `-all` returns the entire collection. `-list_format` returns a Tcl list instead of a collection. `-report_format` is currently ignored.} \
  -arg_help {
    -limit {`count`: Maximum number of objects to return (default 20).}
    -all {Return every object in the collection.}
    -list_format {Return a Tcl list instead of a collection.}
    -report_format {Unsupported; a warning is issued and the flag is ignored.}
  }

sta::define_cmd_args "remove_from_collection" {[-intersect] collection objects} \
  -help {The `remove_from_collection` command returns a new collection with `objects` removed from `collection`. The original collection is not modified. With `-intersect`, the result is the intersection of `collection` and `objects`.} \
  -arg_help {
    -intersect {Return objects that appear in both `collection` and `objects`.}
  }

sta::define_cmd_args "sort_collection" {[-ascending] [-descending] [-dictionary] [-real] [-limit count] collection criteria} \
  -help {The `sort_collection` command returns a new collection sorted by one or more attribute names in `criteria`. The sort is ascending by default. `-dictionary` sorts as strings; numeric sort is the default. `-real` is accepted for compatibility. `-limit` keeps only the first count objects after sorting.} \
  -arg_help {
    -ascending {Sort in ascending order (the default).}
    -descending {Sort in descending order.}
    -dictionary {Compare attribute values as strings.}
    -real {Compare attribute values as numbers (the default). Mutually exclusive with `-dictionary`.}
    -limit {`count`: Keep only the first count objects after sorting.}
  }

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

  # -limit is a count of objects. index_collection uses an inclusive end index.
  if { $limit eq "end" } {
    set result [index_collection $collection 0 end]
  } elseif { [string is integer -strict $limit] && $limit > 0 } {
    set result [index_collection $collection 0 [expr {$limit - 1}]]
  } else {
    set result [index_collection $collection 1 0]
  }

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
