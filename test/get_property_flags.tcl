# Read in design and libraries
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
create_clock -name vclk -period 1000

# Default flag settings (sta_boolean_props_as_int=1, sta_direction_props_short=0)
# Compatible flags used: will return filtered data properly
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

# Default property settings (sta_boolean_props_as_int=1, sta_direction_props_short=0)
# Incompatible property: will return empty lists
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

# Non-default property settings (sta_boolean_props_as_int=0, sta_direction_props_short=1)
# Incompatible property used: will return empty lists
set sta_boolean_props_as_int 0
set sta_direction_props_short 1
puts "TEST 3"
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

# Non-default property settings (sta_boolean_props_as_int=0, sta_direction_props_short=1)
# Compatible property values used: will return filtered data properly
puts "TEST 4"
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
