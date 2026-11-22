# VCD activity on logic that no non-clock input reaches.
# A free-running 2-bit counter has only a clock input, so activity
# propagation from the seeded roots never visits its registers.
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog power_vcd_free_run.v
link_design power_vcd_free_run
create_clock -name clk -period 10 [get_ports clk]
read_vcd -scope tb/dut power_vcd_free_run.vcd
report_activity_annotation
foreach pin_name {r0/Q r1/Q u0/ZN u1/Z r0/D r1/D} {
  puts "$pin_name [get_property [get_pins $pin_name] activity]"
}
report_power
