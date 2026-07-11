read_liberty disable_clock_gating_check.lib
read_verilog disable_clock_gating_check.v
link_design top
create_clock -name clk -period 1.0 [get_ports clk]
set_input_delay -clock clk 0 [get_ports {en d}]

puts "-- libcell --"
set_disable_clock_gating_check [get_lib_cells AND2]
unset_disable_clock_gating_check [get_lib_cells AND2]
puts "ok"

puts "-- inst --"
set_disable_clock_gating_check [get_cells cg]
unset_disable_clock_gating_check [get_cells cg]
puts "ok"

puts "-- pin --"
set_disable_clock_gating_check [get_pins cg/B]
unset_disable_clock_gating_check [get_pins cg/B]
puts "ok"

puts "-- port --"
set_disable_clock_gating_check [get_ports en]
unset_disable_clock_gating_check [get_ports en]
puts "ok"

puts "-- mixed --"
set_disable_clock_gating_check [list [get_lib_cells AND2] [get_cells cg] [get_pins cg/B]]
unset_disable_clock_gating_check [list [get_lib_cells AND2] [get_cells cg] [get_pins cg/B]]
puts "ok"

puts "-- back-compat alias --"
set_disable_inferred_clock_gating [get_cells cg]
unset_disable_inferred_clock_gating [get_cells cg]
puts "ok"

puts "-- write_sdc --"
set_disable_clock_gating_check [list [get_lib_cells AND2] [get_cells cg] [get_pins cg/B]]
write_sdc results/disable_clock_gating_check.sdc
set fp [open results/disable_clock_gating_check.sdc r]
foreach line [split [read $fp] "\n"] {
  if { [string match "*disable_clock_gating_check*" $line] } {
    puts $line
  }
}
close $fp
puts "ok"
