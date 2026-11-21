# Power activity and leakage: clock gate test enable, slews longer than the
# clock period, power after load changes, when conditions that name outputs.
read_liberty asap7_small.lib.gz
read_liberty asap7_seq.lib.gz
read_verilog power_formula_fixes.v
link_design power_formula_fixes
create_clock -name clk -period 100 [get_ports clk]
set_input_delay 0 -clock clk [all_inputs -no_clocks]
set_power_activity -input_ports {en te a b} -activity 0.1 -duty 0.5
set_power_activity -pins u1/Y -activity 0.9 -duty 0.5
# Slews on gclk1 and n1 are longer than the clock period. Density 2e10 is
# two transitions per cycle. The ideal clock gclk1 uses its ideal slew, and
# the annotation on u1/Y is not limited.
set_load 50 [get_nets {gclk1 n1}]
puts [get_property [get_pins u1/Y] activity]
# Gated clock duty is clk duty x P(EN or SE); cg0/SE is tied 0.
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
unset_case_analysis [get_ports en]
puts [get_property [get_pins cg0/GCLK] activity]
# Leakage when conditions of u1 and u2 name the output Y.
report_power -instances {u1 u2}
# Power is recomputed after set_load.
set_load 20 [get_nets y]
report_power -instances {u1 u2}
# A propagated clock uses the gclk1 slew, which limits its activity to
# 1/slew (warning).
set_propagated_clock [get_ports clk]
puts [get_property [get_pins cg1/GCLK] activity]
