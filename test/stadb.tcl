# .stadb smoke: analyzed example1 write/read round trip.
#
# Restore checks run in a child so the cold and warm sessions are isolated.
# Completeness of SDC/liberty/graph lives in the other stadb_* tests.

source stadb_helpers.tcl

set stadb_file [make_result_file "stadb.stadb"]

set stadb_sdc {create_clock -name clk -period 10.123456789 {clk1 clk2 clk3}
set_propagated_clock clk
set_input_delay -clock clk -max 1.234567891 {in1 in2}
set_output_delay -clock clk -min 0.87654321 [get_ports out]
set_false_path -from [get_ports in1] -to [get_ports out]}

set stadb_build "read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
$stadb_sdc"
set stadb_restore "read_sta_db $stadb_file"
set stadb_report "report_checks -digits 6 -path_delay min_max -unconstrained"

eval $stadb_build
sta::find_timing -full_update
write_sta_db $stadb_file
puts "snapshot smaller than liberty: [expr {
  [file size $stadb_file] < [file size ../examples/nangate45_slow.lib.gz] }]"

set cold [stadb_run "$stadb_build\n$stadb_report" smoke_cold]
set warm [stadb_run "$stadb_restore\n$stadb_report" smoke_warm]
stadb_check "search timing" $cold $warm

set replace_file [make_result_file "stadb.replace.stadb"]
set replaced [stadb_run "$stadb_build
sta::find_timing -full_update
write_sta_db $replace_file
read_sta_db $replace_file
$stadb_report" smoke_replace]
stadb_check "search replace" $cold $replaced

set cold_verilog [make_result_file "stadb.cold.v"]
set warm_verilog [make_result_file "stadb.warm.v"]
stadb_run "$stadb_build\nwrite_verilog $cold_verilog" smoke_cv
stadb_run "$stadb_restore\nwrite_verilog $warm_verilog" smoke_wv
stadb_check_files "network verilog" $cold_verilog $warm_verilog

set cold_sdc [make_result_file "stadb.cold.sdc"]
set warm_sdc [make_result_file "stadb.warm.sdc"]
stadb_run "$stadb_build\nwrite_sdc -no_timestamp $cold_sdc" smoke_cs
stadb_run "$stadb_restore\nwrite_sdc -no_timestamp $warm_sdc" smoke_ws
stadb_check_files "sdc write_sdc" $cold_sdc $warm_sdc

set stadb_file2 [make_result_file "stadb.2.stadb"]
set stadb_file3 [make_result_file "stadb.3.stadb"]
stadb_run "$stadb_restore\nwrite_sta_db $stadb_file2" smoke_id2
stadb_run "read_sta_db $stadb_file2\nwrite_sta_db $stadb_file3" smoke_id3
puts "byte idempotent: [expr {
  [stadb_contents $stadb_file2] eq [stadb_contents $stadb_file3] }]"

proc stadb_edit_reports { log } {
  return [lrange [regexp -inline {BEFORE_EDIT(.*)AFTER_EDIT(.*)} $log] 1 2]
}

set stadb_edit {puts BEFORE_EDIT
report_edges -from u1/A -digits 6
replace_cell u1 BUF_X2
puts AFTER_EDIT
report_edges -from u1/A -digits 6}

set cold_edit [stadb_run "$stadb_build\n$stadb_edit" smoke_ce]
set warm_edit [stadb_run "$stadb_restore\n$stadb_edit" smoke_we]
lassign [stadb_edit_reports $warm_edit] warm_before warm_after
puts "restored delays exist before edit: [regexp {[1-9]} $warm_before]"
puts "replace_cell perturbs restored timing: [expr {
  $warm_before ne $warm_after }]"
stadb_check "graph replace_cell" $cold_edit $warm_edit
