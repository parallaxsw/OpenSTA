# Search section: arrivals and case-analysis sim values.
# Generated clocks plus a search write is unsupported (internal exceptions
# are not in the SDC section), so that path is asserted as error 2743.

source stadb_helpers.tcl

set search_file [make_result_file "stadb_search.stadb"]
set search_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
set_case_analysis 0 in2}
set search_dump {foreach clk [get_clocks *] {
  stadb_dump_clock $clk
}
foreach pin {in1 in2 u1/A u1/Z r1/Q r2/D r3/D} {
  stadb_dump_pin_timing [stadb_pin $pin]
}
report_constant [stadb_pin in2]}

stadb_run "$search_build
sta::find_timing -full_update
write_sta_db $search_file" s_w
set cold [stadb_run "$search_build
sta::find_timing -full_update
$search_dump" s_c]
set warm [stadb_run "read_sta_db $search_file
$search_dump" s_r]
stadb_check "search dump" $cold $warm

set gen_file [make_result_file "stadb_search.gen.stadb"]
set gen_out [stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
create_generated_clock -name gclk -source clk1 -divide_by 2 \
  \[get_pins r1/Q\]
set_input_delay -clock clk 0 {in1 in2}
sta::find_timing -full_update
if { \[catch { write_sta_db $gen_file } msg\] } {
  puts \$msg
}" s_gen]
puts "search generated_clock write: [stadb_scrub $gen_out]"
