read_liberty ../examples/sky130hd_tt.lib
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

puts [get_full_names [add_to_collection [get_ports req_*] [get_ports resp_*]]]

set resp_ports [get_ports req_*]
append_to_collection resp_ports [get_ports resp_*]
puts [get_full_names $resp_ports]

puts [get_full_names [filter_collection [get_ports resp_*] "direction==output " -quiet]]
puts [get_full_names [copy_collection [get_ports req_*]]]
foreach_in_collection port [get_ports req_*] {
  puts "foreach_in_collection port: [get_full_name $port]"
}
puts [get_full_names [index_collection [get_ports req_*] 1]]
puts [get_full_names [remove_from_collection [get_ports re*] [get_ports req_*]]]
puts [get_collection_size [get_ports req_*]]
puts [sizeof_collection [get_ports req_*]]
puts [get_full_names [query_collection [get_ports req_*]]]
