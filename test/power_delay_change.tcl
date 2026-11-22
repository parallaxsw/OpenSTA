# report_power is recomputed after delays change.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
report_power -instances {u1 u2}
set_load 20 [get_nets u2z]
report_power -instances {u1 u2}
