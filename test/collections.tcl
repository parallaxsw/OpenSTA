read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

proc assert {expression message} {  
  if { ![uplevel 1 [list expr $expression]] } {
    puts stderr "$message"
  }
}

set collection [get_ports req_*]
assert {[sta::is_collection $collection] == $sta_enable_collections} "get_ports returned incorrect format: $collection"

puts "\tget_collection_size: [get_collection_size $collection]"
puts "\tsizeof_collection: [sizeof_collection $collection]"

puts "\tforeach_in_collection"
foreach_in_collection port $collection {
  puts "foreach_in_collection port: [get_full_name $port]"
}

puts "\tadd_to_collection: "
set add_result [add_to_collection $collection [get_ports resp_*]]
assert {[sta::is_collection $add_result] == $sta_enable_collections} "add_to_collection returned incorrect format: $add_result"
report_object_full_names $add_result

puts "\tcopy_collection: "
set copy_result [copy_collection $collection]
assert {[sta::is_collection $copy_result] == $sta_enable_collections} "copy_collection returned incorrect format: $copy_result"
report_object_full_names $copy_result


puts -nonewline "\tappend_to_collection: sizeof_collection "
set appendable [copy_collection $collection]
append_to_collection appendable [get_ports resp_*]
# llength should be 1 for collection mode, but sizeof_collection should be the same
if {$sta_enable_collections} {
  assert {[llength $appendable] == 1} "append_to_collection returned multiple elements in collection mode"
} else {
  assert {[llength $appendable] > 1} "append_to_collection returned only one element in normal mode"
}
puts "[sizeof_collection $appendable]"
report_object_full_names $appendable

# size should not change
puts -nonewline "\tappend_to_collection -unique: size changed? "
set before [sizeof_collection $appendable]
append_to_collection -unique appendable [get_ports resp_*]
puts "[expr $before != [sizeof_collection $appendable]]"

# appending to an unset variable should auto-initialize it (create the collection)
puts "\tappend_to_collection (auto-initialize unset variable): "
unset -nocomplain fresh
append_to_collection fresh [get_ports req_*]
assert {[info exists fresh]} "append_to_collection did not create the unset variable"
assert {[sta::is_collection $fresh] == $sta_enable_collections} "append_to_collection created incorrect format: $fresh"
puts "\tsizeof_collection: [sizeof_collection $fresh]"
report_object_full_names $fresh

puts "\tindex_collection (single): "
set index_single_result [index_collection $collection 1]
assert {[sta::is_collection $index_single_result] == $sta_enable_collections} "index_collection returned incorrect format: $index_single_result"
report_object_full_names $index_single_result

puts "\tindex_collection (slice): "
set index_slice_result [index_collection $collection 1 5]
assert {[sta::is_collection $index_slice_result] == $sta_enable_collections} "index_collection returned incorrect format: $index_slice_result"
report_object_full_names $index_slice_result

puts "\tremove_from_collection: "
report_object_full_names [remove_from_collection [get_ports re*] $collection]

puts "\tremove_from_collection -intersect: "
report_object_full_names [remove_from_collection -intersect [get_ports re*] [get_ports req_*]]

puts "\tquery_collection: "
report_object_full_names [query_collection [get_ports req_*]]

puts "\tfilter_collection: "
set filter_result [filter_collection [get_ports res*] "direction==output " -quiet]
assert {[sta::is_collection $filter_result] == $sta_enable_collections} "filter_result returned incorrect format: $filter_result"
report_object_full_names $filter_result

puts "\tsort_collection ascending by full_name: "
foreach_in_collection p [sort_collection [get_ports resp_msg*] full_name] {
  puts [get_full_name $p]
}

puts "\tsort_collection descending by full_name: "
foreach_in_collection p [sort_collection -descending [get_ports resp_msg*] full_name] {
  puts [get_full_name $p]
}

puts "\tsort_collection ascending (explicit) by full_name: "
foreach_in_collection p [sort_collection -ascending [get_ports resp_msg*] full_name] {
  puts [get_full_name $p]
}

puts "\tsort_collection on empty collection: "
foreach_in_collection p [sort_collection [get_ports -quiet nonexistent*] full_name] {
  puts [get_full_name $p]
}
