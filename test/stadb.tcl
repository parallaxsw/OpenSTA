# .stadb write/read round trip.
#
# Most restore checks run in a child so the cold and warm sessions are isolated
# from this process's own liberty/netlist. read_sta_db clears the live session
# first, so a same-process replace is also exercised below.

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

# Same-process replace: build, snapshot, then read_sta_db over the live session.
set replace_file [make_result_file "stadb.replace.stadb"]
set replaced [stadb_run "$stadb_build
sta::find_timing -full_update
write_sta_db $replace_file
read_sta_db $replace_file
$stadb_report"]
puts "replace clears live session: [expr { $cold eq $replaced }]"
if { $cold ne $replaced } {
  puts "--- baseline ---\n$cold"
  puts "--- replaced ---\n$replaced"
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

################################################################

# Liberty fidelity beyond path timing. report_checks never touches integrated
# clock-gate recognition (is_clock_gate needs the cell has_clk_gate_* bits
# derived from port attrs) or the internal/leakage power tables that feed
# report_power.
set liberty_attrs {foreach cell_name {CLKGATE_X1 CLKGATETST_X1 BUF_X1 DFF_X1 AND2_X1} {
  set c [get_lib_cells */$cell_name]
  puts "$cell_name is_clock_gate=[get_property $c is_clock_gate] is_integrated=[get_property $c is_integrated_clock_gating_cell] is_buffer=[get_property $c is_buffer] is_sequential=[get_property $c is_sequential] is_inverter=[get_property $c is_inverter] area=[get_property $c area]"
}}

set cold_liberty [stadb_run "$stadb_build\n$liberty_attrs"]
set warm_liberty [stadb_run "$stadb_restore\n$liberty_attrs"]
puts "liberty cell attrs match baseline: [expr { $cold_liberty eq $warm_liberty }]"
if { $cold_liberty ne $warm_liberty } {
  puts "--- baseline ---\n$cold_liberty"
  puts "--- restored ---\n$warm_liberty"
}
puts "baseline has clock gate cell: [regexp {CLKGATE_X1 is_clock_gate=1} $cold_liberty]"

set cold_power [stadb_run "$stadb_build\nreport_power -digits 6"]
set warm_power [stadb_run "$stadb_restore\nreport_power -digits 6"]
puts "power matches baseline: [expr { $cold_power eq $warm_power }]"
if { $cold_power ne $warm_power } {
  puts "--- baseline ---\n$cold_power"
  puts "--- restored ---\n$warm_power"
}

# Instance-level is_clock_gate needs a design that actually instantiates an
# ICG; example1 does not.
set cg_file [make_result_file "stadb.clkgate.stadb"]
set cg_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog stadb_clkgate.v
link_design top
create_clock -name clk -period 10 clk
set_input_transition 0.1 [all_inputs]}
set cg_report {puts "inst is_clock_gate=[get_property [get_cells cg] is_clock_gate]"
puts "lib is_clock_gate=[get_property [get_lib_cells */CLKGATE_X1] is_clock_gate]"
report_power -digits 6}

stadb_run "$cg_build
sta::find_timing -full_update
write_sta_db $cg_file"
set cold_cg [stadb_run "$cg_build\n$cg_report"]
set warm_cg [stadb_run "read_sta_db $cg_file\n$cg_report"]
puts "clock gate example matches baseline: [expr { $cold_cg eq $warm_cg }]"
if { $cold_cg ne $warm_cg } {
  puts "--- baseline ---\n$cold_cg"
  puts "--- restored ---\n$warm_cg"
}
puts "clock gate example sees ICG instance: [regexp {inst is_clock_gate=1} $cold_cg]"

################################################################

# Mode defs, OCV derates and test_cell are absent from nangate45. A tiny lib
# carries all three so restore must round-trip them for timing and power.
set fidelity_file [make_result_file "stadb.fidelity.stadb"]
set fidelity_cold_lib [make_result_file "stadb.fidelity.cold.lib"]
set fidelity_warm_lib [make_result_file "stadb.fidelity.warm.lib"]
set fidelity_build {read_liberty stadb_fidelity.lib
read_verilog stadb_fidelity.v
link_design top
create_clock -name clk -period 10 clk
set_input_transition 0.1 [all_inputs]
set_case_analysis 0 se}
set fidelity_report {report_checks -digits 4 -path_delay min_max -unconstrained
report_power -digits 4}

stadb_run "$fidelity_build
sta::find_timing -full_update
write_sta_db $fidelity_file"
set cold_fidelity [stadb_run "$fidelity_build
$fidelity_report
sta::write_liberty \[get_libs *\] $fidelity_cold_lib"]
set warm_fidelity [stadb_run "read_sta_db $fidelity_file
$fidelity_report
sta::write_liberty \[get_libs *\] $fidelity_warm_lib"]
# Strip the write_liberty filenames (they differ) before comparing reports.
regsub -all $fidelity_cold_lib $cold_fidelity {LIB} cold_fidelity
regsub -all $fidelity_warm_lib $warm_fidelity {LIB} warm_fidelity
puts "fidelity example matches baseline: [expr {
  $cold_fidelity eq $warm_fidelity }]"
if { $cold_fidelity ne $warm_fidelity } {
  puts "--- baseline ---\n$cold_fidelity"
  puts "--- restored ---\n$warm_fidelity"
}
puts "fidelity liberty dump matches: [expr {
  [stadb_contents $fidelity_cold_lib] eq [stadb_contents $fidelity_warm_lib] }]"
