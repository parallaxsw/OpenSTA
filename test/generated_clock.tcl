#Get Object name
interp alias {} get_object_name {} get_full_name

# Get attribute
sta::define_cmd_args "get_attribute" {args}

proc get_attribute {args} {
  sta::parse_key_args "get_attribute" args keys {} flags {-quiet}
  set quiet [info exists flags(-quiet)]
  set arg1 [lindex $args 0]
  set arg2 [lindex $args 1]

  # Suppress unknown property warning
  if { $quiet } {
    suppress_msg 9000
  }
  if { [sta::is_object $arg1] } {
    set result [get_property $arg1 $arg2]
  } elseif { [sta::is_object $arg2] } {
    set result [get_property $arg2 $arg1]
  } else {
    if { $quiet } {
      unsuppress_msg 9000
    }
    error "get_attribute: invalid object $arg1 or $arg2"
  }
  # Re-enable warning after the call
  if { $quiet } {
    unsuppress_msg 9000
  }
  return $result
}

read_liberty generated_clock.lib
read_verilog generated_clock.v
link_design generated_clock
create_clock -name clk -period 10 [get_ports CLK_IN_1] 
create_clock -name clk2 -period 100 [get_ports CLK_IN_2] 

# Should see 9 clocks
puts "Number of clocks: [ llength [get_clocks]]"

# Report all clock periods
foreach clk [get_clocks] {
  puts "[get_object_name $clk] period: [get_attribute $clk period]"
}

# Use TCL command to create a generated clock from generated clock
create_generated_clock \
  -name clk_manual \
  -source [get_pins clk_edge_shift/CLK_OUT] \
  -master_clock clk_edge_shift/CLK_OUT \
  -divide_by 2 \
  [get_ports CLK_OUT_2]

# Should see 10 clocks
puts "Number of clocks: [ llength [get_clocks]]"

# Use command to validate waveforms
report_clock_properties
