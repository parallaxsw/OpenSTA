# .stadb write/read round trip.
#
# The restore runs in a child sta because reading it back into this session
# would leave the parsed liberty and linked netlist sitting next to it, so the
# restored objects would never be the ones reported.

source helpers.tcl

set stadb_file [make_result_file "stadb.stadb"]

# Runs body in a child sta and returns its combined output.
proc stadb_run { body } {
  set script [make_result_file "stadb.child.tcl"]
  set stream [open $script "w"]
  puts $stream "$body\nexit"
  close $stream
  if { [catch { exec [info nameofexecutable] -no_init -no_splash -exit \
                  $script 2>@1 } output] } {
    return "child failed: $output"
  }
  return $output
}

proc stadb_contents { filename } {
  set stream [open $filename "rb"]
  set contents [read $stream]
  close $stream
  return $contents
}

# Splits a child log into the reports either side of the replace_cell.
proc stadb_edit_reports { log } {
  return [lrange [regexp -inline {BEFORE_EDIT(.*)AFTER_EDIT(.*)} $log] 1 2]
}

################################################################

# Spans many command kinds because the sdc section is a tagged record stream: a
# kind whose writer and reader disagree desynchronizes it and fails loudly. The
# values carry more digits than a float holds, which only survives the round
# trip if the section really is binary rather than sdc text.
set stadb_sdc {create_clock -name clk -period 10.123456789 {clk1 clk2 clk3}
create_clock -name vclk -period 7.7 -waveform {1.3 4.9}
create_generated_clock -name gclk -source clk1 -divide_by 3 -add \
  -master_clock clk [get_pins r2/Q]
set_propagated_clock clk
set_clock_transition -rise -max 0.234567891 clk
set_clock_uncertainty -setup 0.111111111 clk
set_clock_uncertainty -from clk -to vclk -hold 0.0987654321
set_clock_latency -max 0.55555 clk
set_clock_latency -source -early -max 0.333333 clk
set_clock_groups -name grps -asynchronous -group {clk} -group {vclk}
set_input_delay -clock clk -max 1.234567891 {in1 in2}
set_output_delay -clock clk -min 0.87654321 [get_ports out]
set_false_path -from [get_ports in1] -to [get_ports out]
set_multicycle_path -setup 2 -from clk -to clk
set_max_delay 3.14159265 -from [get_ports in2]
set_min_delay 0.271828182 -to [get_ports out]
group_path -name grp -from [get_ports in1]
set_case_analysis 0 in2
set_logic_dc in1
set_load -pin_load 0.0123456789 [get_ports out]
set_load 0.00987654 [get_nets r1q]
set_resistance 12.3456 [get_nets r1q]
set_drive 4.56789 in1
set_driving_cell -lib_cell BUF_X1 -pin Z -input_transition_rise 0.0345 in2
set_input_transition 0.0456789 in1
set_timing_derate -early 0.912345
set_timing_derate -late -cell_delay 1.087654
set_disable_timing -from A1 -to ZN u2
set_data_check -from r1/CK -to r2/D -setup 0.135791
set_clock_gating_check -setup 0.024680
set_min_pulse_width -high 0.246813
set_max_transition 0.5432 [current_design]
set_max_capacitance 0.0234 [current_design]
set_max_fanout 12 [current_design]
set_max_area 12345.678
set_max_leakage_power 0.00012345
set_max_dynamic_power 0.0006789}

# Everything a cold run does to reach a constrained, linked session. One
# read_sta_db replaces all of it.
set stadb_build "read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
$stadb_sdc"
set stadb_restore "read_sta_db $stadb_file"
set stadb_report "report_checks -digits 6 -path_delay min_max -unconstrained"

################################################################

# Snapshot a session that has already been analyzed, which is the case worth
# caching: graph, levels and delays are all in the file.
sta::reset_sta_db_counters_cmd
eval $stadb_build
sta::find_timing -full_update
write_sta_db $stadb_file
puts "snapshot smaller than liberty: [expr {
  [file size $stadb_file] < [file size ../examples/nangate45_slow.lib.gz] }]"

array set counters [sta::sta_db_counters_cmd]
puts "cold parses liberty cells: [expr { $counters(liberty_cells_parsed) > 0 }]"

set cold [stadb_run "$stadb_build\n$stadb_report"]
set warm [stadb_run "$stadb_restore\n$stadb_report"]
puts "timing matches baseline: [expr { $cold eq $warm }]"
if { $cold ne $warm } {
  puts "--- baseline ---\n$cold"
  puts "--- restored ---\n$warm"
}

# Timing alone would still match if the restored netlist had lost a hierarchy
# level or a dangling net, so compare the netlist structurally.
set cold_verilog [make_result_file "stadb.cold.v"]
set warm_verilog [make_result_file "stadb.warm.v"]
stadb_run "$stadb_build\nwrite_verilog $cold_verilog"
stadb_run "$stadb_restore\nwrite_verilog $warm_verilog"
puts "netlist matches baseline: [expr {
  [stadb_contents $cold_verilog] eq [stadb_contents $warm_verilog] }]"

# Same argument for the constraints: matching slack does not prove that a
# constraint the reports never reach came back. write_sdc walks all of them.
set cold_sdc [make_result_file "stadb.cold.sdc"]
set warm_sdc [make_result_file "stadb.warm.sdc"]
stadb_run "$stadb_build\nwrite_sdc -no_timestamp $cold_sdc"
stadb_run "$stadb_restore\nwrite_sdc -no_timestamp $warm_sdc"
puts "constraints match baseline: [expr {
  [stadb_contents $cold_sdc] eq [stadb_contents $warm_sdc] }]"

# Skip proof. Identical reports would also come from a restore that quietly
# reparsed the liberty or rebuilt the graph. Reporting inside the child matters
# because a rebuild happens lazily on the first query, not during the read.
set warm_counters [stadb_run "$stadb_restore
$stadb_report
puts \[sta::sta_db_counters_cmd\]"]
array set restored [lindex [split $warm_counters "\n"] end]
puts "restore parses no liberty cells: [expr { $restored(liberty_cells_parsed) == 0 }]"
puts "restore builds no graph: [expr { $restored(graph_vertices_made) == 0 }]"
puts "restore runs no levelization: [expr { $restored(levelize_runs) == 0 }]"
puts "restore computes no delays: [expr { $restored(dcalc_vertices_computed) == 0 }]"
puts "restore visits no search vertices: [expr {
  $restored(search_vertices_visited) == 0 }]"

# Writing a restored session reproduces the file, which is what catches a writer
# and reader that disagree about a field. Measured from the first restore rather
# than against the cold file: the search interns tags in whatever order its
# parallel arrival passes finish, so two cold runs do not agree on tag numbering
# and neither can serve as a reference. A restore replays that pool in file
# order, so everything downstream of it is reproducible.
set stadb_file2 [make_result_file "stadb.2.stadb"]
set stadb_file3 [make_result_file "stadb.3.stadb"]
stadb_run "$stadb_restore\nwrite_sta_db $stadb_file2"
stadb_run "read_sta_db $stadb_file2\nwrite_sta_db $stadb_file3"
puts "byte idempotent: [expr {
  [stadb_contents $stadb_file2] eq [stadb_contents $stadb_file3] }]"

# Liveness: a restored session must still recalculate, which is the annotated
# bit rule. A restore that froze delays would pass every diff above.
set stadb_edit {puts BEFORE_EDIT
report_edges -from u1/A -digits 6
replace_cell u1 BUF_X2
puts AFTER_EDIT
report_edges -from u1/A -digits 6}

set cold_edit [stadb_run "$stadb_build\n$stadb_edit"]
set warm_edit [stadb_run "$stadb_restore\n$stadb_edit"]
lassign [stadb_edit_reports $warm_edit] warm_before warm_after
puts "restored delays exist before edit: [regexp {[1-9]} $warm_before]"
puts "replace_cell perturbs restored timing: [expr { $warm_before ne $warm_after }]"
puts "replace_cell matches baseline: [expr { $cold_edit eq $warm_edit }]"

################################################################

# Instance attributes, on a second design because example1 is hand written and
# carries none. This netlist is synthesizer output, so every instance has the
# source line it came from, which report_json prints as verilog_src. The write
# runs in a child too, since this session is already holding example1.
set attrs_file [make_result_file "stadb.attrs.stadb"]
set attrs_build "read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog verilog_attribute.v
link_design counter
create_clock -name clk -period 10 clk"
set attrs_report "report_checks -format json -digits 4"

stadb_run "$attrs_build
sta::find_timing -full_update
write_sta_db $attrs_file"

# The whole json report, not just the attribute lines, since a restore that
# dropped an instance would change verilog_src without the attribute being at
# fault.
set cold_attrs [stadb_run "$attrs_build\n$attrs_report"]
set warm_attrs [stadb_run "read_sta_db $attrs_file\n$attrs_report"]
puts "instance attributes match baseline: [expr { $cold_attrs eq $warm_attrs }]"
if { $cold_attrs ne $warm_attrs } {
  puts "--- baseline ---\n$cold_attrs"
  puts "--- restored ---\n$warm_attrs"
}

# Guards against the comparison above passing because both sides are empty.
puts "baseline reports source attributes: [regexp {"verilog_src": "[^"]} $cold_attrs]"
