# disconnect_pin call for a pin where SDC is defined
read_liberty asap7_small.lib.gz
read_verilog disconnect_pin_with_sdc.v
link_design top

create_clock -name clk -period 500 clk

# This SDC defines setup and hold time requirements for data pins
# relative to a clock, typical for a source-synchronous interface.
set_data_check -from clk -to [get_pins u0/A] -setup 10
set_data_check -from clk -to [get_pins u0/A] -hold  10
set_data_check -from clk -to [get_pins u1/A] -setup 10
set_data_check -from clk -to [get_pins u1/A] -hold  10
set_multicycle_path -end   -setup 1 -to [get_pins u0/A]
set_multicycle_path -end   -setup 1 -to [get_pins u1/A]
set_multicycle_path -start -hold  0 -to [get_pins u0/A]
set_multicycle_path -start -hold  0 -to [get_pins u1/A]

set in_pin [get_pins u1/A]
set in_net [get_net -of $in_pin]
disconnect_pin $in_net $in_pin
