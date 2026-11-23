# Propagated activity is limited to 1/slew, except on ideal clocks and
# annotated pins.
read_liberty asap7_small.lib.gz
read_liberty asap7_seq.lib.gz
read_verilog power_slew_limit.v
link_design power_slew_limit
create_clock -name clk -period 100 [get_ports clk]
set_power_activity -input_ports {en a} -activity 0.1 -duty 0.5
set_power_activity -pins u1/Y -activity 0.9 -duty 0.5
# Slews on gclk and y are longer than the clock period. Density 1e10 is one
# transition per cycle.
set_load 50 [get_nets {gclk y}]
# The ideal clock gclk uses its ideal slew, and the annotation on u1/Y is
# not limited.
puts [get_property [get_pins cg/GCLK] activity]
puts [get_property [get_pins u1/Y] activity]
# A propagated clock uses the gclk slew, which limits its activity to
# 1/slew (warning).
set_propagated_clock [get_ports clk]
puts [get_property [get_pins cg/GCLK] activity]
