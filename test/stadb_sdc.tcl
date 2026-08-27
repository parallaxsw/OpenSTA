# Kitchen-sink SDC: one distinctive value per stored DbSdcKind.
# Wireload commands are omitted: SDC stores the model name, but liberty
# wireload objects are not serialized, so restore would fail looking them up.

source stadb_helpers.tcl

set stadb_file [make_result_file "stadb_sdc.stadb"]
set cold_sdc [make_result_file "stadb_sdc.cold.sdc"]
set warm_sdc [make_result_file "stadb_sdc.warm.sdc"]

set stadb_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
set_operating_conditions -analysis_type on_chip_variation slow
set_voltage -min 0.95 1.05
set_voltage -object_list [get_nets r1q] 0.99
set_pvt u1 -process 1.1 -voltage 1.02 -temperature 80
create_clock -name clk -period 10.123456789 {clk1 clk2 clk3}
create_clock -name vclk -period 7.7 -waveform {1.3 4.9}
create_generated_clock -name gclk -source clk1 -divide_by 3 -add \
  -master_clock clk [get_pins r2/Q]
create_generated_clock -name eclk -source clk3 -edges {1 3 5} \
  -edge_shift {0.1 0.2 0.3} [get_pins r3/Q]
set_propagated_clock clk
set_propagated_clock [get_pins r1/CK]
set_clock_transition -rise -max 0.234567891 clk
set_clock_uncertainty -setup 0.111111111 clk
set_clock_uncertainty -setup 0.055 [get_pins r1/CK]
set_clock_uncertainty -from clk -to vclk -hold 0.0987654321
set_max_transition -clock_path 0.4321 [get_clocks clk]
set_clock_latency -max 0.55555 clk
set_clock_latency -source -early -max 0.333333 clk
set_clock_groups -name grps -asynchronous -group {clk} -group {vclk}
set_sense -type clock -positive -clocks clk [get_pins u1/A]
set_clock_gating_check -setup 0.024680
set_clock_gating_check -setup 0.031 clk
set_clock_gating_check -setup 0.042 [get_cells u2]
set_clock_gating_check -setup 0.053 [get_pins u2/A1]
set_input_delay -clock clk -max 1.234567891 {in1 in2}
set_output_delay -clock clk -min 0.87654321 [get_ports out]
set_false_path -from [get_ports in1] -to [get_ports out]
set_multicycle_path -setup 2 -from clk -to clk
set_max_delay 3.14159265 -from [get_ports in2]
set_min_delay 0.271828182 -to [get_ports out]
set_path_margin -setup 0.5 -to [get_pins r3/D]
group_path -name grp -from [get_ports in1]
set_data_check -from r1/CK -to r2/D -setup 0.135791
set_disable_timing [get_ports in1]
set_disable_timing [get_lib_pins */BUF_X1/A]
set_disable_timing -from A -to Z [get_lib_cells */BUF_X1]
set_disable_timing -from A1 -to ZN u2
set_disable_clock_gating_check [get_cells u2]
set_case_analysis 0 in2
set_logic_dc in1
set_load -pin_load 0.0123456789 [get_ports out]
set_load 0.00987654 [get_nets r1q]
set_port_fanout_number 7 [get_ports out]
set_resistance 12.3456 [get_nets r1q]
set_drive 4.56789 in1
set_driving_cell -lib_cell BUF_X1 -pin Z -input_transition_rise 0.0345 in2
set_input_transition 0.0456789 in1
set_timing_derate -early 0.912345
set_timing_derate -late -cell_delay 1.087654 [get_cells u1]
set_timing_derate -early -net_delay 0.95 [get_nets r1q]
set_timing_derate -late -cell_delay 1.02 [get_lib_cells */AND2_X1]
set_max_transition 0.5432 [current_design]
set_max_transition 0.321 [get_ports out]
set_max_capacitance 0.0234 [current_design]
set_max_capacitance 0.011 [get_pins u1/Z]
set_min_capacitance 0.001 [get_ports in1]
set_max_fanout 12 [current_design]
set_max_fanout 4 [get_ports in1]
set_max_time_borrow 0.15 [get_clocks clk]
set_max_time_borrow 0.07 [get_pins r1/Q]
set_min_pulse_width -high 0.246813
set_min_pulse_width 0.11 [get_clocks clk]
set_min_pulse_width 0.09 [get_pins r1/CK]
set_max_area 12345.678
set_max_leakage_power 0.00012345
set_max_dynamic_power 0.0006789
set_max_lol 42}

set dump {foreach clk [get_clocks *] { stadb_dump_clock $clk }}

set wout [stadb_run "$stadb_build
write_sta_db $stadb_file
write_sdc -no_timestamp $cold_sdc" sdc_write]
if { ![file exists $stadb_file] } {
  puts $wout
}

set cold [stadb_run "$stadb_build
$dump" sdc_cdump]
set warm [stadb_run "read_sta_db $stadb_file
$dump" sdc_wdump]
stadb_check "sdc clocks" $cold $warm

stadb_run "$stadb_build
write_sdc -no_timestamp $cold_sdc" sdc_csdc
stadb_run "read_sta_db $stadb_file
write_sdc -no_timestamp $warm_sdc" sdc_wsdc
if { [file exists $cold_sdc] && [file exists $warm_sdc] } {
  stadb_check_sdc $cold_sdc $warm_sdc
} else {
  puts "sdc write_sdc matches: 0"
  puts "  fix: stadb/DbSdc.cc"
}
