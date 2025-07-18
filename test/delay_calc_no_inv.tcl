# sta_no_inv_delay_calc variable should ignore inverter delays

# sta_no_inv_delay_calc is off
read_liberty asap7_invbuf.lib.gz
read_verilog delay_calc_no_inv.v
link_design delay_calc_no_inv
create_clock -name clk -period 1
set_input_delay -clock clk 0 [all_inputs -no_clocks]
set_output_delay -clock clk 0 [all_outputs]
report_checks

# sta_no_inv_delay_calc is on
set sta_no_inv_delay_calc 1
read_verilog delay_calc_no_inv.v
link_design delay_calc_no_inv
create_clock -name clk -period 1
set_input_delay -clock clk 0 [all_inputs -no_clocks]
set_output_delay -clock clk 0 [all_outputs]
report_checks
