# SDC/Innovus all_inputs / all_outputs -clock / -edge_triggered.
source helpers.tcl
read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top

create_clock -name clk -period 10 {clk1 clk2 clk3}
create_clock -name vclk -period 10

proc show { label coll } {
  puts $label
  report_object_full_names $coll
}

puts "before delays"
show "all_inputs" [all_inputs]
show "all_inputs -no_clocks" [all_inputs -no_clocks]
show "all_inputs -edge_triggered" [all_inputs -edge_triggered]
show "all_outputs -edge_triggered" [all_outputs -edge_triggered]

set_input_delay -clock clk 1.0 in1
set_output_delay -clock clk 1.0 out

puts "after in1/out delays"
show "all_inputs -edge_triggered" [all_inputs -edge_triggered]
show "all_inputs -clock clk" [all_inputs -clock clk]
show "all_outputs -edge_triggered" [all_outputs -edge_triggered]
show "all_outputs -clock clk" [all_outputs -clock clk]
show "all_outputs -clock vclk" [all_outputs -clock vclk]

# Typical Innovus flow: default 0 delay on unconstrained non-clock inputs.
set ALL_IN_EXCEPT_CLK [all_inputs -no_clocks]
set INPUT_PORTS_WITHOUT_DELAY \
  [remove_from_collection $ALL_IN_EXCEPT_CLK [all_inputs -edge_triggered]]
show "INPUT_PORTS_WITHOUT_DELAY" $INPUT_PORTS_WITHOUT_DELAY
if { [sizeof_collection $INPUT_PORTS_WITHOUT_DELAY] > 0 } {
  set_input_delay 0 -clock vclk $INPUT_PORTS_WITHOUT_DELAY
}
show "all_inputs -edge_triggered after default" [all_inputs -edge_triggered]
show "all_inputs -clock vclk" [all_inputs -clock vclk]
show "all_inputs -clock clk -edge_triggered" \
  [all_inputs -clock clk -edge_triggered]

# Clockless delay (time 0) is still a stored constraint.
set_input_delay 2.0 in2
puts "after clockless in2"
show "all_inputs -edge_triggered" [all_inputs -edge_triggered]
show "all_inputs -clock vclk" [all_inputs -clock vclk]
show "all_inputs -clock clk" [all_inputs -clock clk]

# Reference-pin delay without -clock has delay->clock() == nullptr.
set_input_delay -reference_pin clk1 1.5 in2
puts "after ref_pin in2"
show "all_inputs -edge_triggered" [all_inputs -edge_triggered]
show "all_inputs -clock clk" [all_inputs -clock clk]
set ALL_IN_EXCEPT_CLK [all_inputs -no_clocks]
set INPUT_PORTS_WITHOUT_DELAY \
  [remove_from_collection $ALL_IN_EXCEPT_CLK [all_inputs -edge_triggered]]
show "INPUT_PORTS_WITHOUT_DELAY after ref_pin" $INPUT_PORTS_WITHOUT_DELAY
