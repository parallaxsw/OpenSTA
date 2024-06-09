read_liberty ../examples/sky130hd_tt.lib
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

set req_ports [get_ports req_*]
add_to_collection req_ports [get_ports resp_*]
puts [get_full_names $req_ports]

set resp_ports [get_ports req_*]
append_to_collection resp_ports [get_ports resp_*]
puts [get_full_names $resp_ports]

puts [compare_collections [get_ports req_*] [get_ports resp_*]]
puts [compare_collections [get_ports req_*] [get_ports req_*]]
puts [get_full_names [filter_collection [get_ports resp_*] "direction==out " -quiet]]
puts [get_full_names [copy_collection [get_ports req_*]]]
foreach_in_collection port [get_ports req_*] {
  puts "foreach_in_collection port: [get_full_name $port]"
}
puts [get_full_names [index_collection [get_ports req_*] 1]]
puts [get_full_names [remove_from_collection [get_ports re*] [get_ports req_*]]]
puts [sizeof_collection [get_ports req_*]]
puts [get_full_names [query_objects [get_ports req_*]]]
