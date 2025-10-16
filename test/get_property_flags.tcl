# get_* -filter with property flags

# Read in design and libraries
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
create_clock -name vclk -period 1000

puts "TEST 1"
puts "get_clocks"
report_object_full_names [get_clocks -filter is_virtual==0 *]
puts "get_clocks 2"
report_object_full_names [get_clocks -filter is_virtual==1 *]
puts "get_lib_cells"
report_object_full_names [get_lib_cells -filter is_buffer==1 *]
puts "get_lib_cells 2"
report_object_full_names [get_lib_cells -filter is_inverter==0 *]
puts "get_pins"
report_object_full_names [get_pins -filter direction==input *]
puts "get_pins 2"
report_object_full_names [get_pins -filter direction==output *]
puts "get_ports"
report_object_full_names [get_ports -filter direction==input *]
puts "get_ports 2"
report_object_full_names [get_ports -filter direction==output *]

puts "TEST 2"
puts "get_clocks"
report_object_full_names [get_clocks -filter is_virtual==false *]
puts "get_clocks 2"
report_object_full_names [get_clocks -filter is_virtual==true *]
puts "get_lib_cells"
report_object_full_names [get_lib_cells -filter is_buffer==true *]
puts "get_lib_cells 2"
report_object_full_names [get_lib_cells -filter is_inverter==false *]
puts "get_pins"
report_object_full_names [get_pins -filter direction==in *]
puts "get_pins 2"
report_object_full_names [get_pins -filter direction==out *]
puts "get_ports"
report_object_full_names [get_ports -filter direction==in *]
puts "get_ports 2"
report_object_full_names [get_ports -filter direction==out *]
