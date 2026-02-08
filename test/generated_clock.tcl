read_liberty generated_clock.lib
read_verilog generated_clock.v
link_design generated_clock
create_clock -name clk -period 10 [get_ports CLK_IN_1] 
create_clock -name clk2 -period 100 [get_ports CLK_IN_2] 

# Should see 9 clocks
puts "Number of clocks: [ llength [get_clocks]]"

# Report all clock periods
foreach_in_collection clk [get_clocks] {
  puts "[get_object_name $clk] period: [get_attribute $clk period]"
}

# Use TCL command to create a generated clock from generated clock
create_generated_clock \
  -name clk_manual \
  -source [get_pins clk_edge_shift/CLK_OUT] \
  -master_clock clk_edge_shift/CLK_OUT \
  -divide_by 2 \
  [get_ports CLK_OUT_2]

# Should see 10 clocks
puts "Number of clocks: [ llength [get_clocks]]"

# Use command to validate waveforms
report_clock_properties
