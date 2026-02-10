# SDC port lookup for escaped bus names.

# Load design and create clock.
read_verilog sdc_strip_escaped_bus.v
link_design sdc_strip_escaped_bus
create_clock -name clk -period 1000

# The netlist ports are escaped names like \a[0] and \y[0].
# SDC names "a"/"y" should match those escaped bus names.
set_input_delay -clock clk 0 {a}
set_output_delay -clock clk 0 {y}
set_input_delay -clock clk 1 {a[0]}
set_output_delay -clock clk 1 {y[0]}
