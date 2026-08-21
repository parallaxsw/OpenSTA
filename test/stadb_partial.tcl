# Partial snapshots: liberty-only, linked without a graph, graph without search.

source stadb_helpers.tcl

set lib_file [make_result_file "stadb_partial.lib.stadb"]
set lib_dump {foreach cell {BUF_X1 DFF_X1 AND2_X1} {
  stadb_dump_liberty_cell [get_lib_cells */$cell]
}}
stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
write_sta_db $lib_file" p_lw
set cold_l [stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
$lib_dump" p_lc]
set warm_l [stadb_run "read_sta_db $lib_file
$lib_dump" p_lr]
stadb_check "liberty only" $cold_l $warm_l

set net_file [make_result_file "stadb_partial.net.stadb"]
set net_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}}
set net_dump {foreach inst [get_cells *] {
  stadb_dump_instance $inst
}
foreach clk [get_clocks *] {
  stadb_dump_clock $clk
}}
stadb_run "$net_build
write_sta_db $net_file" p_nw
set cold_n [stadb_run "$net_build
$net_dump" p_nc]
set warm_n [stadb_run "read_sta_db $net_file
$net_dump" p_nr]
stadb_check "network no graph" $cold_n $warm_n

# create_generated_clock levelizes (graph, no arrivals).
set graph_file [make_result_file "stadb_partial.graph.stadb"]
set graph_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
create_generated_clock -name gclk -source clk1 -divide_by 2 \
  [get_pins r1/Q]}
set graph_dump {foreach clk [get_clocks *] {
  stadb_dump_clock $clk
}
report_edges -from [get_pins r1/Q] -digits 4}
stadb_run "$graph_build
write_sta_db $graph_file" p_gw
set cold_g [stadb_run "$graph_build
$graph_dump" p_gc]
set warm_g [stadb_run "read_sta_db $graph_file
$graph_dump" p_gr]
stadb_check "graph no search" $cold_g $warm_g
