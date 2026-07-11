# Register Q with set_power_activity must be used by findActivity
# (activity_map_ is not seeded for user-annotated seq outputs).
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}

set_power_activity -pins [get_pins r1/Q] -density 0.2 -duty 0.5
report_activity_annotation -report_annotated
report_power -digits 6 -instances r1
