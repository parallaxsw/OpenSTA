# sdc clock annotation for 2d array

# Load design and create clock
set sta_continue_on_error 1
read_verilog sdc_strip_escaped_bus.v
link_design sdc_strip_escaped_bus
create_clock -name clk -period 1000

# sta_strip_escaped_bus 0: should produce errors for { a } and { y }
set sta_strip_escaped_bus 0
set_input_delay -clock clk 3 { a }
set_output_delay -clock clk 3 { y }
set_input_delay -clock clk 4 { a[0] }
set_output_delay -clock clk 4 { y[0] }

# sta_strip_escaped_bus 1
set sta_strip_escaped_bus 1
set_input_delay -clock clk 0 { a }
set_output_delay -clock clk 0 { y }
set_input_delay -clock clk 1 { a[0] }
set_output_delay -clock clk 1 { y[0] }
