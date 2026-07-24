# remove_input_delay/remove_output_delay (and reset_* aliases) with a collection
# passed directly (not wrapped in a list). Regression for collection argument
# handling that previously only worked when the collection was wrapped with
# [list ...].
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd
create_clock -name clk -period 10 [get_ports clk]

set clk [lindex [get_clocks clk] 0]

# Count the set_input_delay/set_output_delay lines currently in effect.
proc port_delay_count { sdc_cmd } {
  write_sdc results/remove_input_delay.sdc
  set fp [open results/remove_input_delay.sdc r]
  set count 0
  foreach line [split [read $fp] "\n"] {
    if { [string match "$sdc_cmd*" $line] } { incr count }
  }
  close $fp
  return $count
}

proc input_delay_count {} {
  return [port_delay_count "set_input_delay"]
}

proc output_delay_count {} {
  return [port_delay_count "set_output_delay"]
}

puts "-- single-bit port, collection passed directly --"
set port [get_ports req_msg[0]]
set_input_delay -clock [get_object_name $clk] 100 $port
puts "after set:             [input_delay_count]"
remove_input_delay -clock [get_object_name $clk] $port
puts "after remove (direct): [input_delay_count]"
puts "ok"

puts "-- single-bit port, collection wrapped in a list --"
set_input_delay -clock [get_object_name $clk] 100 $port
remove_input_delay -clock [get_object_name $clk] [list $port]
puts "after remove (list):   [input_delay_count]"
puts "ok"

puts "-- multi-bit bus, collection passed directly --"
set bus [get_ports req_msg*]
set_input_delay -clock [get_object_name $clk] 100 $bus
puts "after set:             [input_delay_count]"
remove_input_delay -clock [get_object_name $clk] $bus
puts "after remove (direct): [input_delay_count]"
puts "ok"

puts "-- multi-bit bus, collection wrapped in a list --"
set_input_delay -clock [get_object_name $clk] 100 $bus
remove_input_delay -clock [get_object_name $clk] [list $bus]
puts "after remove (list):   [input_delay_count]"
puts "ok"

puts "-- reset_input_delay alias, collection passed directly --"
set_input_delay -clock [get_object_name $clk] 100 $bus
reset_input_delay -clock [get_object_name $clk] $bus
puts "after reset (direct):  [input_delay_count]"
puts "ok"

puts "-- output: single-bit port, collection passed directly --"
set oport [get_ports resp_msg[0]]
set_output_delay -clock [get_object_name $clk] 100 $oport
puts "after set:             [output_delay_count]"
remove_output_delay -clock [get_object_name $clk] $oport
puts "after remove (direct): [output_delay_count]"
puts "ok"

puts "-- output: single-bit port, collection wrapped in a list --"
set_output_delay -clock [get_object_name $clk] 100 $oport
remove_output_delay -clock [get_object_name $clk] [list $oport]
puts "after remove (list):   [output_delay_count]"
puts "ok"

puts "-- output: multi-bit bus, collection passed directly --"
set obus [get_ports resp_msg*]
set_output_delay -clock [get_object_name $clk] 100 $obus
puts "after set:             [output_delay_count]"
remove_output_delay -clock [get_object_name $clk] $obus
puts "after remove (direct): [output_delay_count]"
puts "ok"

puts "-- output: multi-bit bus, collection wrapped in a list --"
set_output_delay -clock [get_object_name $clk] 100 $obus
remove_output_delay -clock [get_object_name $clk] [list $obus]
puts "after remove (list):   [output_delay_count]"
puts "ok"

puts "-- reset_output_delay alias, collection passed directly --"
set_output_delay -clock [get_object_name $clk] 100 $obus
reset_output_delay -clock [get_object_name $clk] $obus
puts "after reset (direct):  [output_delay_count]"
puts "ok"
