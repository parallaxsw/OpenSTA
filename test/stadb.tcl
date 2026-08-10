# .stadb write/read round trip.
#
# Restoring runs in a child process on purpose: reading the snapshot back into
# this session would leave the already parsed library and linked netlist
# sitting next to it, so the restored objects would never actually be used and
# the test would prove nothing.
#
# Only derived facts are printed. Echoing the child reports instead would pin
# this golden file to nangate45 timing numbers, which is what delay_calc
# already covers.

set stadb_app [info nameofexecutable]
set stadb_tmp "stadb_tmp[pid]"

# Runs body in a child sta and returns its combined output.
proc stadb_run { body } {
  global stadb_app stadb_tmp
  set script "$stadb_tmp.tcl"
  set stream [open $script "w"]
  puts $stream $body
  puts $stream "exit"
  close $stream
  set failed [catch {
    exec $stadb_app -no_init -no_splash -exit $script 2>@1
  } output]
  file delete -force $script
  if { $failed } {
    return "child failed: $output"
  }
  return $output
}

# Constraints deliberately span many command kinds. The SDC section is a tagged
# record stream, so a kind whose writer and reader disagree desynchronizes the
# stream and fails loudly rather than producing a subtly wrong number.
#
# The periods and delays carry more digits than a float holds on purpose: SDC
# text loses the low bits through unit scaling, so these values only survive a
# round trip if the section really is binary.
set stadb_sdc "create_clock -name clk -period 10.123456789 {clk1 clk2 clk3}
create_clock -name vclk -period 7.7 -waveform {1.3 4.9}
create_generated_clock -name gclk -source clk1 -divide_by 3 -add \
  -master_clock clk \[get_pins r2/Q]
set_propagated_clock clk
set_clock_transition -rise -max 0.234567891 clk
set_clock_uncertainty -setup 0.111111111 clk
set_clock_uncertainty -from clk -to vclk -hold 0.0987654321
set_clock_latency -max 0.55555 clk
set_clock_latency -source -early -max 0.333333 clk
set_clock_groups -name grps -asynchronous -group {clk} -group {vclk}
set_input_delay -clock clk -max 1.234567891 {in1 in2}
set_output_delay -clock clk -min 0.87654321 \[get_ports out]
set_false_path -from \[get_ports in1] -to \[get_ports out]
set_multicycle_path -setup 2 -from clk -to clk
set_max_delay 3.14159265 -from \[get_ports in2]
set_min_delay 0.271828182 -to \[get_ports out]
group_path -name grp -from \[get_ports in1]
set_case_analysis 0 in2
set_logic_dc in1
set_load -pin_load 0.0123456789 \[get_ports out]
set_load 0.00987654 \[get_nets r1q]
set_resistance 12.3456 \[get_nets r1q]
set_drive 4.56789 in1
set_driving_cell -lib_cell BUF_X1 -pin Z -input_transition_rise 0.0345 in2
set_input_transition 0.0456789 in1
set_timing_derate -early 0.912345
set_timing_derate -late -cell_delay 1.087654
set_disable_timing -from A1 -to ZN u2
set_data_check -from r1/CK -to r2/D -setup 0.135791
set_clock_gating_check -setup 0.024680
set_min_pulse_width -high 0.246813
set_max_transition 0.5432 \[current_design]
set_max_capacitance 0.0234 \[current_design]
set_max_fanout 12 \[current_design]
set_max_area 12345.678
set_max_leakage_power 0.00012345
set_max_dynamic_power 0.0006789"

# Everything a cold run does to reach a constrained, linked session. The warm
# run replaces all of it with one read_sta_db, so it never parses liberty or
# verilog, never links, and never re-applies a constraint.
set stadb_build "read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
$stadb_sdc"
set stadb_restore "read_sta_db $stadb_tmp.stadb"

################################################################

sta::reset_sta_db_counters_cmd
eval $stadb_build
# Snapshot a session that has already been analyzed, which is the case worth
# caching: the graph, its levels and its delays are all in the file, so the
# warm run has nothing left to compute.
sta::find_timing -full_update
write_sta_db "$stadb_tmp.stadb"
puts "snapshot smaller than liberty: [expr {
  [file size "$stadb_tmp.stadb"] < [file size ../examples/nangate45_slow.lib.gz] }]"

array set counters [sta::sta_db_counters_cmd]
puts "cold parses liberty cells: [expr { $counters(liberty_cells_parsed) > 0 }]"

# Mode 1 and 2: baseline against write-then-read.
set report "report_checks -digits 6 -path_delay min_max -unconstrained"
set cold [stadb_run "$stadb_build\n$report"]
set warm [stadb_run "$stadb_restore\n$report"]
puts "timing matches baseline: [expr { $cold == $warm }]"
if { $cold != $warm } {
  puts "--- baseline ---\n$cold"
  puts "--- restored ---\n$warm"
}

# Timing alone would still match if the restored netlist had lost a hierarchy
# level or a dangling net, so compare the whole thing structurally.
stadb_run "$stadb_build
write_verilog $stadb_tmp.cold.v"
stadb_run "$stadb_restore
write_verilog $stadb_tmp.warm.v"
set cold_v [open "$stadb_tmp.cold.v" "r"]
set warm_v [open "$stadb_tmp.warm.v" "r"]
puts "netlist matches baseline: [expr { [read $cold_v] == [read $warm_v] }]"
close $cold_v
close $warm_v

# Same argument for the constraints: matching slack does not prove that a
# constraint the reports never reach came back. write_sdc walks all of them.
stadb_run "$stadb_build
write_sdc $stadb_tmp.cold.sdc"
stadb_run "$stadb_restore
write_sdc $stadb_tmp.warm.sdc"

# Strips the header, which carries the date.
proc stadb_sdc_body { filename } {
  set stream [open $filename "r"]
  set body [read $stream]
  close $stream
  return [join [lsearch -all -inline -not -regexp [split $body "\n"] {^#}] "\n"]
}
puts "constraints match baseline: [expr {
  [stadb_sdc_body "$stadb_tmp.cold.sdc"] eq [stadb_sdc_body "$stadb_tmp.warm.sdc"] }]"

# Skip proof. Identical reports would also come from a restore that quietly
# reparsed the liberty or rebuilt the graph, so the counters are the only thing
# that catches it. Reporting inside the child matters: the rebuild would happen
# lazily on the first query, not during the read.
set warm_counters [stadb_run "$stadb_restore
$report
puts \[sta::sta_db_counters_cmd\]"]
array set restored [lindex [split $warm_counters "\n"] end]
puts "restore parses no liberty cells: [expr { $restored(liberty_cells_parsed) == 0 }]"
puts "restore builds no graph: [expr { $restored(graph_vertices_made) == 0 }]"
puts "restore runs no levelization: [expr { $restored(levelize_runs) == 0 }]"
puts "restore computes no delays: [expr { $restored(dcalc_vertices_computed) == 0 }]"
puts "restore visits no search vertices: [expr {
  $restored(search_vertices_visited) == 0 }]"

# Mode 3: writing a restored session reproduces the file, which is what
# catches a writer and reader that disagree about a field.
#
# Measured from the first restore rather than against the cold file. The search
# runs its arrival passes in parallel and interns tags in whatever order the
# threads finish, so two cold runs of the same script do not agree on tag
# numbering and neither can serve as a reference. A restore replays that pool
# in file order, so everything downstream of it is reproducible.
stadb_run "$stadb_restore
write_sta_db $stadb_tmp.2.stadb"
stadb_run "read_sta_db $stadb_tmp.2.stadb
write_sta_db $stadb_tmp.3.stadb"
set first [open "$stadb_tmp.2.stadb" "rb"]
set second [open "$stadb_tmp.3.stadb" "rb"]
puts "byte idempotent: [expr { [read $first] == [read $second] }]"
close $first
close $second

# Liveness: a restored session must still recalculate, which is the annotated
# bit rule. A restore that froze delays would pass every report diff above.
set edit "
puts BEFORE_EDIT
report_edges -from u1/A -digits 6
replace_cell u1 BUF_X2
puts AFTER_EDIT
report_edges -from u1/A -digits 6"

# Splits a child log into the reports either side of the replace_cell.
proc stadb_edit_halves { log } {
  set before [string range $log [string first "BEFORE_EDIT" $log] \
                [expr { [string first "AFTER_EDIT" $log] - 1 }]]
  set after [string range $log [string first "AFTER_EDIT" $log] end]
  return [list $before $after]
}

set cold_edit [stadb_run "$stadb_build\n$edit"]
set warm_edit [stadb_run "$stadb_restore\n$edit"]
lassign [stadb_edit_halves $warm_edit] warm_before warm_after
puts "restored delays exist before edit: [expr { [regexp {[1-9]} $warm_before] == 1 }]"
puts "replace_cell perturbs restored timing: [expr {
  [string range $warm_before 11 end] != [string range $warm_after 10 end] }]"
puts "replace_cell matches baseline: [expr { $cold_edit == $warm_edit }]"

file delete -force "$stadb_tmp.stadb" "$stadb_tmp.2.stadb" "$stadb_tmp.3.stadb" \
  "$stadb_tmp.cold.v" "$stadb_tmp.warm.v" \
  "$stadb_tmp.cold.sdc" "$stadb_tmp.warm.sdc"
