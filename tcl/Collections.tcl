################################################################
#
# Collection commands from Intel Quartus (links below).
# This script applies collection commands to simple TCL lists.
# Link 1: https://www.intel.com/content/www/us/en/docs/programmable/683432/24-2/collection-commands.html
# Link 2: https://www.intel.com/content/www/us/en/docs/programmable/683432/24-2/tcl_pkg_dcmd_dni_ver_1-0.html
# Link 3: https://www.intel.com/content/www/us/en/docs/programmable/683432/24-2/tcl_pkg_sta_ver_1-0_cmd_query_collection.html
#
################################################################

# Add objects to a collection, resulting in a new collection. The base collection remains unchanged. (Link 2)
interp alias {} add_to_collection {} concat

# Duplicates the contents of a collection, resulting in a new collection. The base collection remains unchanged. (Link 2)
interp alias {} copy_collection {} return -level 0

# The foreach_in_collection command is similar to the foreach Tcl command. Use it to iterate through all elements in a collection. (Link 1)
interp alias {} foreach_in_collection {} foreach

# Use the get_collection_size command to get the number of elements in a collection. (Link 1)
interp alias {} get_collection_size {} llength

# Given a collection and an index, if the index is in range, create a new collection containing only the single object. (Link 2)
# Optionally a second index can be passed to create a new collection with the objects between the two indices in the base collection.
interp alias {} index_collection {} lindex

# Returns the number of objects in a collection. (Link 2)
interp alias {} sizeof_collection {} llength

# Sorts a collection based on one or more attributes, resulting in a new, sorted collection. The sort is ascending by default. (Link 2)
interp alias {} sort_collection {} lsort

# Query collection objects. (Link 3)
interp alias {} query_collection {} return -level 0

# Append objects to a collection and modifies a variable. (Link 2)
proc append_to_collection { collection objects } {
  upvar $collection coll
  lappend coll {*}$objects
}

# Remove objects from a collection, resulting in a new collection. The base collection remains unchanged. (Link 2)
proc remove_from_collection { collection objects } {
  foreach object $objects {
    set idx [lsearch -exact $collection $object]
    if { $idx != -1 } {
      set collection [lreplace $collection $idx $idx]
    }
  }
  return $collection
}

# Filters an existing collection, resulting in a new collection. The base collection remains unchanged. (Link 2)
proc filter_collection { args } {
  set args [remove_from_collection $args [list "-quiet"]]
  set collection [lindex $args 0]
  set filter [lindex $args 1]
  if { [llength $collection] == 0 } {
    return $collection
  } else {
    set object_type [object_type $obj]
    if { $object_type == "Pin" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_pins "pin"]
    } elseif { $object_type == "Instance" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_insts "instance"]
    } elseif { $object_type == "Net" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_nets "net"]
    } elseif { $object_type == "Port" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_ports "port"]
    } elseif { $object_type == "Edge" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_edges "edge"]
    } elseif { $object_type == "Clock" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_clocks "clock"]
    } elseif { $object_type == "LibertyCell" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_lib_cells "liberty cell"]
    } elseif { $object_type == "LibertyPort" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_lib_pins "liberty port"]
    } elseif { $object_type == "LibertyLibrary" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_liberty_libraries "liberty library"]
    } elseif { $object_type == "TimingArcSet" } {
      return [filter_objs $keys(-filter) [lindex $collection 0] filter_timing_arcs "timing arc"]
    } else {
      sta_error 100 "unsupported object type $object_type."
    }
  }
}
