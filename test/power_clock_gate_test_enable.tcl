# Clock gate test enable (clock_gate_test_pin) in gated clock activity.
read_liberty asap7_small.lib.gz
read_liberty asap7_seq.lib.gz
read_verilog power_clock_gate_test_enable.v
link_design power_clock_gate_test_enable
create_clock -name clk -period 100 [get_ports clk]
set_power_activity -input_ports {en te} -activity 0.1 -duty 0.5
# Gated clock duty is clk duty x P(EN or SE); cg0/SE is tied 0.
# Density 2e10 is two transitions per cycle.
puts [get_property [get_pins cg0/GCLK] activity]
puts [get_property [get_pins cg1/GCLK] activity]
# EN held at 0: cg0 stops, cg1 runs while SE can be 1.
set_case_analysis 0 [get_ports en]
puts [get_property [get_pins cg0/GCLK] activity]
puts [get_property [get_pins cg1/GCLK] activity]
# SE without a driver is ignored, so cg1 stops.
disconnect_pin te cg1/SE
puts [get_property [get_pins cg1/GCLK] activity]
connect_pin te cg1/SE
puts [get_property [get_pins cg1/GCLK] activity]
# Activities propagate through en again after unset_case_analysis.
unset_case_analysis [get_ports en]
puts [get_property [get_pins cg0/GCLK] activity]
