# Liberty record round trip: kitchen-sink flags, ICG, fidelity dump.

source stadb_helpers.tcl

set dump_kitchen {foreach cell [lsort -dictionary [stadb_names [get_lib_cells *]]] {
  set lib_cell [get_lib_cells $cell]
  stadb_dump_liberty_cell $lib_cell
  foreach_in_collection port [get_lib_pins $cell/*] {
    stadb_dump_liberty_port $port
  }
}}

set kitchen_file [make_result_file "stadb_liberty.kitchen.stadb"]
set kitchen_cold_lib [make_result_file "stadb_liberty.kitchen.cold.lib"]
set kitchen_warm_lib [make_result_file "stadb_liberty.kitchen.warm.lib"]
set kitchen_build {read_liberty stadb_kitchen.lib
read_verilog stadb_kitchen.v
link_design top
create_clock -name clk -period 10 clk}

stadb_run "$kitchen_build
sta::find_timing -full_update
write_sta_db $kitchen_file" lib_kw
set cold_k [stadb_run "$kitchen_build
$dump_kitchen
sta::write_liberty \[get_libs *\] $kitchen_cold_lib" lib_kc]
set warm_k [stadb_run "read_sta_db $kitchen_file
$dump_kitchen
sta::write_liberty \[get_libs *\] $kitchen_warm_lib" lib_kr]
regsub -all $kitchen_cold_lib $cold_k {LIB} cold_k
regsub -all $kitchen_warm_lib $warm_k {LIB} warm_k
stadb_check "liberty kitchen cells" $cold_k $warm_k
stadb_check_backup "liberty kitchen write_liberty" $kitchen_cold_lib \
  $kitchen_warm_lib

set dump_ng {foreach cell_name {CLKGATE_X1 CLKGATETST_X1 BUF_X1 DFF_X1 AND2_X1} {
  stadb_dump_liberty_cell [get_lib_cells */$cell_name]
}
report_power -digits 6}

set ng_file [make_result_file "stadb_liberty.ng.stadb"]
set ng_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}}
stadb_run "$ng_build
sta::find_timing -full_update
write_sta_db $ng_file" lib_ngw
set cold_ng [stadb_run "$ng_build
$dump_ng" lib_ngc]
set warm_ng [stadb_run "read_sta_db $ng_file
$dump_ng" lib_ngr]
stadb_check "liberty nangate45 cells" $cold_ng $warm_ng

set cg_file [make_result_file "stadb_liberty.cg.stadb"]
set cg_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog stadb_clkgate.v
link_design top
create_clock -name clk -period 10 clk
set_input_transition 0.1 [all_inputs]}
set cg_dump {puts "liberty inst cg is_clock_gate [stadb_prop [get_cells cg] is_clock_gate]"
puts "liberty cell CLKGATE_X1 is_clock_gate [stadb_prop [get_lib_cells */CLKGATE_X1] is_clock_gate]"
report_power -digits 6}
stadb_run "$cg_build
sta::find_timing -full_update
write_sta_db $cg_file" lib_cgw
set cold_cg [stadb_run "$cg_build
$cg_dump" lib_cgc]
set warm_cg [stadb_run "read_sta_db $cg_file
$cg_dump" lib_cgr]
stadb_check "liberty clock_gate" $cold_cg $warm_cg

set fid_file [make_result_file "stadb_liberty.fid.stadb"]
set fid_cold [make_result_file "stadb_liberty.fid.cold.lib"]
set fid_warm [make_result_file "stadb_liberty.fid.warm.lib"]
set fid_build {read_liberty stadb_fidelity.lib
read_verilog stadb_fidelity.v
link_design top
create_clock -name clk -period 10 clk
set_input_transition 0.1 [all_inputs]
set_case_analysis 0 se}
stadb_run "$fid_build
sta::find_timing -full_update
write_sta_db $fid_file" lib_fw
stadb_run "$fid_build
sta::write_liberty \[get_libs *\] $fid_cold" lib_fc
stadb_run "read_sta_db $fid_file
sta::write_liberty \[get_libs *\] $fid_warm" lib_fr
stadb_check_files "liberty fidelity write_liberty" $fid_cold $fid_warm
