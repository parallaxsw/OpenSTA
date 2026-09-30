# Ports created before sta::init_sta must keep their directions.
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
sta::init_sta
puts "inputs: [llength [all_inputs]] outputs: [llength [all_outputs]]"
report_checks -format end
