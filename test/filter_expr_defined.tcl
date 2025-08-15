# check filtering with defined(<property>) and undefined(<property>)

read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}

set sta_direction_props_short 1

set data_input_ports_list [filter_collection [all_inputs] "direction == in && undefined(clocks)"]
set data_output_ports_list [filter_collection [all_outputs] "direction == out && undefined(clocks)"]
set clock_input_ports_list [filter_collection [all_inputs] "direction == in && defined(clocks)"]
set clock_output_ports_list [filter_collection [all_outputs] "direction == out && defined(clocks)"]

set wildcard_nets [get_nets -filter "full_name =~ r?q"]

puts "Data input ports:"
report_object_full_names $data_input_ports_list
puts "--------------------------------"
puts "Data output ports:"
report_object_full_names $data_output_ports_list
puts "--------------------------------"
puts "Clock input ports:"
report_object_full_names $clock_input_ports_list
puts "--------------------------------"
puts "Clock output ports:"
report_object_full_names $clock_output_ports_list
puts "--------------------------------"
puts "Wildcard nets for r?q:"
report_object_full_names $wildcard_nets
puts "--------------------------------"
