# set_path_margin: per-path slack adjustment on the capture clock.

read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

proc setup_at { args } {
  report_checks {*}$args -path_delay max -digits 4 -fields {} -group_path_count 1
}
proc hold_at { args } {
  report_checks {*}$args -path_delay min -digits 4 -fields {} -group_path_count 1
}
proc report_json_path_margin { label path_delay args } {
  set cmd [concat [list report_checks] $args \
    [list -path_delay $path_delay -format json -group_path_count 1]]
  with_output_to_variable json $cmd
  if { [regexp {"path_margin": ([^,\n]+)} $json match margin] } {
    puts "$label $margin"
  } else {
    puts "$label none"
  }
}

# Test -to and that -setup and -hold are properly applied.
set_path_margin -setup 0.50 -comment {tighten setup time} -to [get_pins r3/D]
setup_at -to [get_pins r3/D]
report_json_path_margin setup_to_tighten max -to [get_pins r3/D]
set_path_margin -hold 0.50 -comment {tighten hold time} -to [get_pins r3/D]
hold_at -to [get_pins r3/D]
report_json_path_margin hold_to_tighten min -to [get_pins r3/D]
set_path_margin -setup -67 -comment {loosen setup time} -to [get_pins r3/D]
setup_at -to [get_pins r3/D]
report_json_path_margin setup_to_loosen max -to [get_pins r3/D]
set_path_margin -hold -0.50 -comment {loosen hold time} -to [get_pins r3/D]
hold_at -to [get_pins r3/D]
report_json_path_margin hold_to_loosen min -to [get_pins r3/D]

# Test -from
unset_path_exceptions -through [get_pins u1/Z]
set_path_margin -setup 2.0 -from [get_pins r1/CK]
# Should see path margin.
setup_at -from [get_pins r1/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_r1 max -from [get_pins r1/CK] -to [get_pins r3/D]
# Should not see path margin.
setup_at -from [get_pins r2/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_r2 max -from [get_pins r2/CK] -to [get_pins r3/D]

# Test -from and -to.
unset_path_exceptions -to [get_pins r3/D]
set_path_margin -setup 5.0 -from [get_pins r1/CK] -to [get_pins r3/D]
# Should see path margin.
setup_at -from [get_pins r1/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_to_r1 max -from [get_pins r1/CK] -to [get_pins r3/D]
# Should not see path margin.
setup_at -from [get_pins r2/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_to_r2 max -from [get_pins r2/CK] -to [get_pins r3/D]

# Test -through.
unset_path_exceptions -to [get_pins r3/D]
set_path_margin -setup 3.0 -through [get_pins u1/Z]
# Should not see path margin.
setup_at -from [get_pins r1/CK] -to [get_pins r3/D]
report_json_path_margin setup_through_r1 max -from [get_pins r1/CK] -to [get_pins r3/D]
# Should see path margin.
setup_at -from [get_pins r2/CK] -to [get_pins r3/D]
report_json_path_margin setup_through_r2 max -from [get_pins r2/CK] -to [get_pins r3/D]

# Test a clock-scoped startpoint.
unset_path_exceptions -from [get_pins r1/CK]
unset_path_exceptions -through [get_pins u1/Z]
set_path_margin -setup 4.0 -from [get_clocks clk]
# Should see path margin on the clock.
setup_at -from [get_pins r1/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_clk_r1 max -from [get_pins r1/CK] -to [get_pins r3/D]
setup_at -from [get_pins r2/CK] -to [get_pins r3/D]
report_json_path_margin setup_from_clk_r2 max -from [get_pins r2/CK] -to [get_pins r3/D]

# Test -from, -through, and -to.
unset_path_exceptions -from [get_clocks clk]
set_path_margin -setup 6.0 -from [get_pins r1/CK] \
                           -through [get_pins u2/A1] \
                           -to [get_pins r3/D]
# Should see path margin.
setup_at -from [get_pins r1/CK] -to [get_pins r3/D]
report_json_path_margin setup_combo_r1 max -from [get_pins r1/CK] -to [get_pins r3/D]
# Should not see path margin.
setup_at -from [get_pins r2/CK] -to [get_pins r3/D]
report_json_path_margin setup_combo_r2 max -from [get_pins r2/CK] -to [get_pins r3/D]
