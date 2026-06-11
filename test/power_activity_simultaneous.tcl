# Check simultaneous input transitions during activity propagation.
read_liberty power_activity_simultaneous.lib
read_verilog power_activity_simultaneous.v
link_design top

create_clock -name clk -period 1 clk
set_power_activity -input_ports {a b} -activity 0.5 -duty 0.5

puts "a [get_property [get_ports a] activity]"
puts "b [get_property [get_ports b] activity]"
puts "u1/Z [get_property [get_pins u1/Z] activity]"
