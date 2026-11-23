# Leakage when conditions that name output pins. The conditions of u1
# (BUFx2) and u2 (AND2x2) name Y; their duties add to 1, so leakage matches
# the library's unconditional leakage_power value.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
report_power -instances {u1 u2}
