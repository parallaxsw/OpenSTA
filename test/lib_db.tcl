# Per-library .libdb cache. Liberty-only write/read, and write after STA
# structures are already loaded.

source stadb_helpers.tcl

set dump {foreach cell {BUF_X1 DFF_X1 AND2_X1} {
  stadb_dump_liberty_cell [get_lib_cells */$cell]
}
foreach_in_collection port [get_lib_pins */BUF_X1/*] {
  stadb_dump_liberty_port $port
}}

set lib_file [make_result_file "lib_db.nangate.libdb"]
stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
write_lib_db \[get_libs *\] $lib_file" libdb_w
set cold_l [stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
$dump" libdb_lc]
set warm_l [stadb_run "read_lib_db $lib_file
$dump" libdb_lr]
stadb_check "lib_db liberty only" $cold_l $warm_l

# Dump one library after the design is linked and timed.
set sta_file [make_result_file "lib_db.linked.libdb"]
set sta_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}}
stadb_run "$sta_build
sta::find_timing -full_update
write_lib_db \[get_libs *\] $sta_file" libdb_sw
set report {report_checks -digits 4 -path_delay min_max}
set cold_s [stadb_run "$sta_build
$report" libdb_sc]
set warm_s [stadb_run "read_lib_db $sta_file
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
$report" libdb_sr]
stadb_check "lib_db sta after linked write" $cold_s $warm_s
