# moving a loop cut with netlist edits invalidates stale arrivals
# asap7_simple has timing groups on output ports.
suppress_msg 1212
read_liberty asap7_simple.lib.gz
read_liberty asap7_seq.lib.gz
read_verilog levelize_loop_cut_move.v
link_design top
create_clock -name clk -period 1000 [get_ports clk]
set_input_delay -clock clk 0 [get_ports {in0 in1}]
set_output_delay -clock clk 0 [get_ports out0]

proc swap_pins { inst pin1 pin2 } {
  set net1 [get_full_name [get_nets -of_objects [get_pins $inst/$pin1]]]
  set net2 [get_full_name [get_nets -of_objects [get_pins $inst/$pin2]]]
  disconnect_pin $net1 $inst/$pin1
  connect_pin $net2 $inst/$pin1
  disconnect_pin $net2 $inst/$pin2
  connect_pin $net1 $inst/$pin2
}

report_checks -format end
report_disabled_edges
swap_pins g6 C B
report_checks -format end
report_disabled_edges
# Deleting the g4 loop edge relevelizes and moves the g2 loop cut to g6.
swap_pins g4 A B
report_disabled_edges
report_checks
