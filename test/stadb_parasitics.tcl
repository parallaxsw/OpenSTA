# Parasitics are reserved in the format and not written. This test makes
# that drop loud: annotation after restore, and replace_cell vs a SPEF cold run.

source stadb_helpers.tcl

set spef_file [make_result_file "stadb_parasitics.stadb"]
set spef_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
read_spef ../examples/example1.dspef
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}}
set anno {report_parasitic_annotation}
set edit {replace_cell u1 BUF_X2
report_checks -digits 4 -path_delay min_max}

stadb_run "$spef_build
sta::find_timing -full_update
write_sta_db $spef_file" par_w

set cold_anno [stadb_run "$spef_build
$anno" par_ca]
set warm_anno [stadb_run "read_sta_db $spef_file
$anno" par_wa]
if { $cold_anno eq $warm_anno } {
  puts "parasitics status: restored"
} else {
  puts "parasitics status: dropped"
  puts "  fix: stadb/DbFormat.hh DbSectionId::parasitics (not written)"
}

set cold_edit [stadb_run "$spef_build
sta::find_timing -full_update
$edit" par_ce]
set warm_edit [stadb_run "read_sta_db $spef_file
$edit" par_we]
if { $cold_edit eq $warm_edit } {
  puts "parasitics replace_cell: matches_cold"
} else {
  puts "parasitics replace_cell: liberty_only_after_restore"
}
