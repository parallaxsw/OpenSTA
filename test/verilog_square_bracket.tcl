# Verilog input pin with square brackets in middle of pin name
read_liberty asap7_small.lib.gz
read_verilog verilog_square_bracket.v
link_design square_bracket
create_clock -period 1 -name vclk
set_input_delay -clock vclk 1 [all_inputs]
