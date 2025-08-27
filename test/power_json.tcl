# report_power gcd
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

read_sdc ../examples/gcd_sky130hd.sdc
set_propagated_clock clk
read_spef ../examples/gcd_sky130hd.spef
set_power_activity -input -activity .1
set_power_activity -input_port reset -activity 0

report_power -format json
report_power -format json -instances "[get_cells -filter "name=~clkbuf*"]"
