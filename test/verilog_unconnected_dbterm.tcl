read_liberty ../examples/nangate45_typ.lib.gz
read_verilog verilog_unconnected_dbterm.v
link_design top 
puts "Find b1/out2: [get_property [get_pins b1/out2] full_name]"
puts "Find b2/out2: [get_property [get_pins b2/out2] full_name]"
# Check if net is connected to "b2/u3/Z" that was the b2/out2 in parent block
set iterm [sta::find_pin "b2/u3/Z"]
set net [get_net -of_object [get_pin $iterm]]
if { $net != "NULL" } {
  puts "Net connected to b2/u3/Z: [get_full_name $net]"
} else {
  puts "b2/u3/Z is not connected to any net."
}
puts "Find internal net connected to b2/out2: [get_property [get_net b2/out2] full_name]"