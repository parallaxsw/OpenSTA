# Graph section: SDF annotated delays, combinational loops, bidirects.

source stadb_helpers.tcl

set sdf_file [make_result_file "stadb_graph.sdf.stadb"]
set sdf_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
read_sdf ../examples/example1.sdf
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}}
set sdf_dump {foreach pin {u1/A u1/Z r1/Q r2/Q r3/D} {
  report_edges -from [get_pins $pin] -digits 6
}}
stadb_run "$sdf_build
sta::find_timing -full_update
write_sta_db $sdf_file" g_sdfw
set cold_s [stadb_run "$sdf_build
$sdf_dump" g_sdfc]
set warm_s [stadb_run "read_sta_db $sdf_file
$sdf_dump" g_sdfr]
stadb_check "graph sdf edges" $cold_s $warm_s

set loop_file [make_result_file "stadb_graph.loop.stadb"]
set loop_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog stadb_loop.v
link_design top
create_clock -name clk -period 10
set_input_delay -clock clk 0 [get_ports a]}
set loop_dump {report_disabled_edges
foreach pin {u1/A1 u1/A2 u1/ZN u2/A u2/Z} {
  report_edges -from [get_pins $pin] -digits 4
}}
stadb_run "$loop_build
sta::find_timing -full_update
write_sta_db $loop_file" g_lw
set cold_l [stadb_run "$loop_build
$loop_dump" g_lc]
set warm_l [stadb_run "read_sta_db $loop_file
$loop_dump" g_lr]
stadb_check "graph loop" $cold_l $warm_l

set bid_file [make_result_file "stadb_graph.bid.stadb"]
set bid_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog stadb_bidirect.v
link_design top
create_clock -name clk -period 10
set_input_delay -clock clk 0 [get_ports a]
set_output_delay -clock clk 0 [get_ports z]}
set bid_dump {foreach pin {a z u1/A u1/Z} {
  stadb_dump_pin [stadb_pin $pin]
  report_edges -from [stadb_pin $pin] -digits 4
}}
stadb_run "$bid_build
sta::find_timing -full_update
write_sta_db $bid_file" g_bw
set cold_b [stadb_run "$bid_build
$bid_dump" g_bc]
set warm_b [stadb_run "read_sta_db $bid_file
$bid_dump" g_br]
stadb_check "graph bidirect" $cold_b $warm_b
