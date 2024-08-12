################################################################
#
# Collection commands used in commercial EDA tools.
# This script applies collection commands to lists.
# Synopsys: https://www.edaboard.com/attachments/tcl_scripting_language-pdf.91704/
# Intel: https://www.intel.com/content/www/us/en/docs/programmable/683325/18-1/collection-commands.html
# Intel: https://www.intel.com/content/www/us/en/docs/programmable/683432/22-1/tcl_pkg_dcmd_sdc_collection_api_ver_1-0.html
#
################################################################

# Helper functions
proc get_full_names { collection } {
  set full_names {}
  foreach name $collection {
    lappend full_names [get_full_name $name]
  }
  return $full_names
}

# Aliases: collections are just TCL lists
interp alias {} add_to_collection {} append_to_collection
interp alias {} copy_collection {} return -level 0
interp alias {} foreach_in_collection {} foreach
interp alias {} index_collection {} lindex
interp alias {} get_collection_size {} llength
interp alias {} sizeof_collection {} llength
interp alias {} query_collection {} return -level 0
interp alias {} query_objects {} return -level 0

# TODO: sort_collection?

# Collection functions which have no direct TCL equivalent
proc append_to_collection { collection objects } {
  upvar $collection coll
  lappend coll {*}$objects
}
proc compare_collections { collection1 collection2 } {
  set diff {}
  foreach i $collection1 {
    if {[lsearch -exact $collection2 $i]==-1} {
      lappend diff $i
    }
  }
  return [llength $diff]
}
proc remove_from_collection { collection objects } {
  foreach object $objects {
    set idx [lsearch -exact $collection $object]
    if { $idx != -1 } {
      set collection [lreplace $collection $idx $idx]
    }
  }
  return $collection
}
proc filter_collection { args } {
  set args [remove_from_collection $args [list "-quiet"]]
  set collection [lindex $args 0]
  set filter [lindex $args 1]
  if { [llength $collection] == 0 } {
    return $collection
  } else {
    if { [string match "*_p_Instance" [lindex $collection 0]] } {
      get_cells -filter $filter [get_full_names $collection]
    } elseif { [string match "*_p_Port" [lindex $collection 0]] } {
      get_ports -filter $filter [get_full_names $collection]
    } elseif { [string match "*_p_Pin" [lindex $collection 0]] } {
      get_pins -filter $filter [get_full_names $collection]
    } else {
      error "Error: filter_collection on collection $collection has unsupported datatype!"
    }
  }
}
