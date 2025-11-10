# disconnect_pin call for a pin where SDC is defined
read_liberty asap7_invbuf.lib.gz
read_liberty asap7_seq.lib.gz
read_liberty asap7_simple.lib.gz
read_verilog disconnect_pin_with_sdc.v
link_design top

create_clock -name clk -period 500 {clk1 clk2 clk3}
set_input_delay -clock clk 1 {in1 in2}
set_input_transition 10 {in1 in2 clk1 clk2 clk3}
set_propagated_clock {clk1 clk2 clk3}
sta::set_delay_calculator prima

#sta::set_debug exception_merge 10

puts "set_multicycle_path on pins"
set_multicycle_path -end   -setup 1 -to [get_pins {u0/buf0/A}]
set_multicycle_path -end   -setup 1 -to [get_pins {u0/buf1/A}]
set_multicycle_path -end   -setup 1 -to [get_pins {u0/buf2/A}]
set_multicycle_path -end   -setup 1 -to [get_pins {u0/buf3/A}]

puts "disconnect the pin w/ SDC"
set in_pin [get_pins u0/buf2/A]
set in_net [get_net -of $in_pin]
disconnect_pin $in_net $in_pin

puts "Pass"
