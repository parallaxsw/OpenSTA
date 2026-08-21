# Inventory of SDC constraint commands vs what stadb stores.
# A new set_*/create_* in sdc/Sdc.tcl fails this golden until it is
# classified stored (add DbSdcKind write/read) or dropped.

source stadb_helpers.tcl

# cmd -> {stored|dropped  kind_or_note  file}
array set stadb_sdc_map {
  create_clock {stored DbSdcKind::clock stadb/DbSdc.cc}
  create_generated_clock {stored DbSdcKind::generated_clock stadb/DbSdc.cc}
  create_voltage_area {dropped ignored_by_opensta -}
  group_path {stored DbSdcKind::group_path stadb/DbSdc.cc}
  set_case_analysis {stored DbSdcKind::case_value stadb/DbSdc.cc}
  set_clock_gating_check {stored DbSdcKind::clock_gating_check stadb/DbSdc.cc}
  set_clock_groups {stored DbSdcKind::clock_groups stadb/DbSdc.cc}
  set_clock_latency {stored DbSdcKind::clock_latency/clock_insertion stadb/DbSdc.cc}
  set_clock_sense {stored DbSdcKind::clock_sense stadb/DbSdc.cc}
  set_clock_transition {stored DbSdcKind::clock_slew stadb/DbSdc.cc}
  set_clock_uncertainty {stored DbSdcKind::clock_uncertainty stadb/DbSdc.cc}
  set_data_check {stored DbSdcKind::data_check stadb/DbSdc.cc}
  set_disable_clock_gating_check {stored DbSdcKind::disable_gating_check stadb/DbSdc.cc}
  set_disable_inferred_clock_gating {stored DbSdcKind::disable_gating_check stadb/DbSdc.cc}
  set_disable_timing {stored DbSdcKind::disable_* stadb/DbSdc.cc}
  set_drive {stored DbSdcKind::input_drive stadb/DbSdc.cc}
  set_driving_cell {stored DbSdcKind::input_drive stadb/DbSdc.cc}
  set_false_path {stored DbSdcKind::exception stadb/DbSdc.cc}
  set_fanout_load {dropped not_supported -}
  set_hierarchy_separator {dropped session_not_sdc -}
  set_ideal_latency {dropped ignored_by_opensta -}
  set_ideal_net {dropped ignored_by_opensta -}
  set_ideal_network {dropped ignored_by_opensta -}
  set_ideal_transition {dropped ignored_by_opensta -}
  set_input_delay {stored DbSdcKind::input_delay stadb/DbSdc.cc}
  set_input_transition {stored DbSdcKind::input_drive stadb/DbSdc.cc}
  set_level_shifter_strategy {dropped ignored_by_opensta -}
  set_level_shifter_threshold {dropped ignored_by_opensta -}
  set_load {stored DbSdcKind::port_ext_cap/net_wire_cap stadb/DbSdc.cc}
  set_logic_dc {stored DbSdcKind::logic_value stadb/DbSdc.cc}
  set_logic_one {stored DbSdcKind::logic_value stadb/DbSdc.cc}
  set_logic_zero {stored DbSdcKind::logic_value stadb/DbSdc.cc}
  set_max_area {stored DbSdcKind::max_area stadb/DbSdc.cc}
  set_max_capacitance {stored DbSdcKind::cap_limit_* stadb/DbSdc.cc}
  set_max_delay {stored DbSdcKind::exception stadb/DbSdc.cc}
  set_max_dynamic_power {stored DbSdcKind::max_dynamic_power stadb/DbSdc.cc}
  set_max_fanout {stored DbSdcKind::fanout_limit_* stadb/DbSdc.cc}
  set_max_leakage_power {stored DbSdcKind::max_leakage_power stadb/DbSdc.cc}
  set_max_time_borrow {stored DbSdcKind::latch_borrow_limit stadb/DbSdc.cc}
  set_max_transition {stored DbSdcKind::slew_limit_*/clock_slew_limit stadb/DbSdc.cc}
  set_min_capacitance {stored DbSdcKind::cap_limit_* stadb/DbSdc.cc}
  set_min_delay {stored DbSdcKind::exception stadb/DbSdc.cc}
  set_min_pulse_width {stored DbSdcKind::min_pulse_width stadb/DbSdc.cc}
  set_multicycle_path {stored DbSdcKind::exception stadb/DbSdc.cc}
  set_operating_conditions {stored DbSdcKind::operating_conditions stadb/DbSdc.cc}
  set_output_delay {stored DbSdcKind::output_delay stadb/DbSdc.cc}
  set_path_margin {stored DbSdcKind::exception stadb/DbSdc.cc}
  set_port_fanout_number {stored DbSdcKind::port_ext_cap stadb/DbSdc.cc}
  set_propagated_clock {stored DbSdcKind::propagated_clock_pin stadb/DbSdc.cc}
  set_pvt {stored DbSdcKind::instance_pvt stadb/DbSdc.cc}
  set_resistance {stored DbSdcKind::net_resistance stadb/DbSdc.cc}
  set_sense {stored DbSdcKind::clock_sense stadb/DbSdc.cc}
  set_timing_derate {stored DbSdcKind::derating_* stadb/DbSdc.cc}
  set_voltage {stored DbSdcKind::voltage/net_voltage stadb/DbSdc.cc}
  set_wire_load_min_block_size {dropped not_supported -}
  set_wire_load_mode {stored DbSdcKind::wireload_mode stadb/DbSdc.cc}
  set_wire_load_model {stored DbSdcKind::wireload stadb/DbSdc.cc}
  set_wire_load_selection_group {stored DbSdcKind::wireload_selection stadb/DbSdc.cc}
}

proc stadb_parse_define_cmds { filename } {
  set stream [open $filename "r"]
  set text [read $stream]
  close $stream
  set cmds {}
  foreach {all cmd} [regexp -all -inline {define_cmd_args[ \t]+"?([A-Za-z0-9_]+)"?} $text] {
    if { [lsearch -exact $cmds $cmd] < 0 } {
      lappend cmds $cmd
    }
  }
  return $cmds
}

puts "# classify a new command as stored (add DbSdcKind write/read) or dropped"
puts "cmd\tstatus\tkind\tfile"

set files [list [stadb_repo_file .. sdc Sdc.tcl] \
             [stadb_repo_file .. search Search.tcl]]
set cmds {}
foreach file $files {
  foreach cmd [stadb_parse_define_cmds $file] {
    if { ![string match "set_*" $cmd] \
           && ![string match "create_*" $cmd] \
           && $cmd ne "group_path" } {
      continue
    }
    if { [string match "unset_*" $cmd] || [string match "delete_*" $cmd] } {
      continue
    }
    if { [lsearch -exact $cmds $cmd] < 0 } {
      lappend cmds $cmd
    }
  }
}

foreach cmd [lsort $cmds] {
  if { [info exists stadb_sdc_map($cmd)] } {
    lassign $stadb_sdc_map($cmd) status kind file
    puts "$cmd\t$status\t$kind\t$file"
  } else {
    puts "$cmd\tUNKNOWN\tadd stored or dropped row\tstadb/DbSdc.hh"
    puts "stadb inventory: new SDC command $cmd"
    puts "  action: add a row in stadb_sdc_inventory.tcl as stored or dropped"
    puts "  if stored: add DbSdcKind in stadb/DbSdc.hh, write+read in"
    puts "             stadb/DbSdc.cc, and a constraint in test/stadb_sdc.tcl"
  }
}
