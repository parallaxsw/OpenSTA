# write_path_spice deasserts asynchronous set/reset pins (sibling of #474/#475)
source helpers.tcl
read_liberty write_path_spice_async_set_reset.lib.gz
read_verilog write_path_spice_async_set_reset.v
link_design repro
create_clock -name clk -period 10 [get_ports clk]
set_input_delay -clock clk 0 [get_ports {d nset nreset}]
set_output_delay -clock clk 0 [get_ports q]
# The flop's async pins are unconstrained ports, so no constant propagates to
# them.  A CLK -> Q deck must hold both at their deasserted level, which for
# these active low pins (ff: clear "!RESET_B", preset "!SET_B") is VPWR:
#   v1 r0/RESET_B 0 1.800
#   v2 r0/SET_B 0 1.800
# Before the fix neither pin got a value and both fell through to the tie low
# default (0.000), asserting set and reset: q is frozen and never transitions,
# so the deck measures nothing.
set spice_file [make_result_file "write_path_spice_async_set_reset.sp"]
write_path_spice -path_args {-path_delay max -from [get_pins r0/CLK] -to [get_ports q]} \
  -spice_file $spice_file \
  -lib_subckt_file write_path_spice_async_set_reset.cells.spice \
  -model_file write_path_spice_async_set_reset.models.spice \
  -power VPWR -ground VGND \
  -simulator ngspice
report_file ${spice_file}_1.sp
