# Ports created before sta::init_sta must keep their directions.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
sta::init_sta
puts "inputs: [llength [all_inputs]] outputs: [llength [all_outputs]]"
report_checks -format end
