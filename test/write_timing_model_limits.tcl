# write_timing_model ports carry the design rules of the block behind them.
# Prints nothing when they do.
source helpers.tcl

proc check { what ok } {
  if { !$ok } {
    puts "FAIL: $what"
  }
}

# The value of a limit attribute on a port of a written model, "" if none.
proc model_limit { lib_file port attr } {
  set stream [open $lib_file r]
  set text [read $stream]
  close $stream
  set start [string first "pin(\"$port\")" $text]
  set end [string first "timing()" $text $start]
  set body [string range $text $start $end]
  set value ""
  regexp "$attr : (\[0-9.eE+-\]+)" $body ignore value
  return $value
}

proc violations {} {
  return [expr [sta::max_slew_violation_count] \
            + [sta::max_capacitance_violation_count]]
}

read_liberty ../examples/nangate45_typ.lib.gz

# A net between two model instances is checked as the block's own net is.
read_verilog write_timing_model_limits.v
link_design block
create_clock -name clk -period 1 [get_ports clk]
set model_file [make_result_file write_timing_model_limits.lib]
write_timing_model -cell_name block_model $model_file
set_load 200 [get_ports out]
set flat_200 [violations]
set_load 5 [get_ports out]
set flat_5 [violations]

# write_timing_model leaves the model library loaded.
read_verilog write_timing_model_limits.v
link_design parent
create_clock -name clk -period 1 [get_ports clk]
set_load 200 [get_nets n]
check "model 200fF [violations] violations, block $flat_200" \
  [expr ([violations] > 0) == ($flat_200 > 0)]
set_load 5 [get_nets n]
check "model 5fF [violations] violations, block $flat_5" \
  [expr ([violations] > 0) == ($flat_5 > 0)]

# Through resistive wires: meeting a port's limit keeps the pins behind it
# within theirs (u1/A: default_max_transition, u2/Z: max_capacitance).
read_verilog write_timing_model_limits.v
link_design block_rc
read_spef write_timing_model_limits.spef
create_clock -name clk -period 1 [get_ports clk]
set rc_file [make_result_file write_timing_model_limits_rc.lib]
write_timing_model -cell_name block_rc_model $rc_file
set in_limit [model_limit $rc_file in max_transition]
set out_limit [model_limit $rc_file out max_capacitance]
check "in has max_transition" [expr {$in_limit != ""}]
check "out has max_capacitance" [expr {$out_limit != ""}]
if { $in_limit != "" } {
  set_max_transition 0.198535 [current_design]
  set_input_transition $in_limit [get_ports in]
  check "in at $in_limit: u1/A slew [get_property [get_pins u1/A] slew_max_rise]" \
    [expr [sta::max_slew_violation_count] == 0]
}
if { $out_limit != "" } {
  # Less the writer's rounding.
  set_load [expr $out_limit - 0.001] [get_ports out]
  check "out at $out_limit: [sta::max_capacitance_violation_count] violations" \
    [expr [sta::max_capacitance_violation_count] == 0]
}
