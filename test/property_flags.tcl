# Property aliases backed by existing Liberty, SDC, and search APIs.
read_liberty gf180mcu_sram.lib.gz
read_liberty asap7_small.lib.gz
read_verilog get_is_memory.v
link_design get_is_memory
create_clock -name clk -period 1 CLK

set dff [get_lib_cells DFFHQx4_ASAP7_75t_R]
set dff_clk [get_lib_pins DFFHQx4_ASAP7_75t_R/CLK]

puts "lib has_timing_model [get_property $dff has_timing_model]"
puts "lib is_sequential [get_property $dff is_sequential]"
puts "lib dont_use [get_property $dff dont_use]"
puts "lib pin_capacitance [get_property $dff_clk pin_capacitance]"
puts "lib is_clock [get_property $dff_clk is_clock]"
puts "lib is_clock_port [get_property $dff_clk is_clock_port]"
puts "lib is_clock_pin [get_property $dff_clk is_clock_pin]"
puts "inst is_sequential [get_property [get_cells dff_inst] is_sequential]"
puts "inst is_memory [get_property [get_cells sram_inst] is_memory]"
puts "pin is_clock [get_property [get_pins dff_inst/CLK] is_clock]"
puts "pin is_clock_pin [get_property [get_pins dff_inst/CLK] is_clock_pin]"
puts "pin is_register_clock [get_property [get_pins dff_inst/CLK] is_register_clock]"
get_property [get_ports CLK] pin_capacitance
get_property [get_ports CLK] clocks
get_property [get_ports CLK] clock_domains
get_property [get_ports CLK] clockDomains
puts "port timing properties evaluated"
