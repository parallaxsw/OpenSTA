# write_timing_model with -scalar option
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog verilog_attribute.v
link_design counter
create_clock -name clk -period 0 clk
set_input_delay -clock clk 0 [all_inputs -no_clocks]
set_output_delay -clock clk 0 [all_outputs]
set_load 1 [all_outputs]
write_timing_model -scalar results/write_timing_model_scalar.log
