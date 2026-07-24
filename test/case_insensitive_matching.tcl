# Test the sta_case_insensitive_matching variable.
# The design uses lower case object names (ports clk1/in1, instances r1/u1,
# nets r1q, ...) so upper case patterns only match when the variable is on.
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name core_clk -period 1 {clk1 clk2 clk3}

# Print the sorted full names matched by a get_* command.
proc show { label cmd } {
  set names {}
  foreach_in_collection obj [eval $cmd] {
    lappend names [get_full_name $obj]
  }
  puts "$label = {[lsort $names]}"
}

# Print the sorted names of matched liberty libraries.
proc show_libs { label cmd } {
  set names {}
  foreach_in_collection lib [eval $cmd] {
    lappend names [get_name $lib]
  }
  puts "$label = {[lsort $names]}"
}

puts "######## sta_case_insensitive_matching = 0 (default) ########"
show "get_ports clk1       " {get_ports -quiet clk1}
show "get_ports CLK1       " {get_ports -quiet CLK1}
show "get_ports CLK*       " {get_ports -quiet CLK*}
show "get_cells R1         " {get_cells -quiet R1}
show "get_pins  R1/CK      " {get_pins -quiet R1/CK}
show "get_nets  R1Q        " {get_nets -quiet R1Q}
show "get_clocks CORE_CLK  " {get_clocks -quiet CORE_CLK}
show_libs "get_libs NANGATE*   " {get_libs -quiet NANGATE*}

puts ""
puts "######## sta_case_insensitive_matching = 1 ########"
set sta_case_insensitive_matching 1
show "get_ports CLK1       " {get_ports -quiet CLK1}
show "get_ports CLK*       " {get_ports -quiet CLK*}
show "get_ports In2        " {get_ports -quiet In2}
show "get_cells R1         " {get_cells -quiet R1}
show "get_cells U*         " {get_cells -quiet U*}
show "get_pins  R1/CK      " {get_pins -quiet R1/CK}
show "get_pins  U1/*       " {get_pins -quiet U1/*}
show "get_nets  R1Q        " {get_nets -quiet R1Q}
show "get_clocks CORE_CLK  " {get_clocks -quiet CORE_CLK}
show "get_clocks CORE_*    " {get_clocks -quiet CORE_*}
show "get_lib_cells */dff_x1" {get_lib_cells -quiet */dff_x1}
show "get_lib_pins */dff_x1/ck" {get_lib_pins -quiet */dff_x1/ck}
show "get_cells -filter =~U*" {get_cells -quiet -filter "full_name=~U*"}
show "get_cells -filter ==R1" {get_cells -quiet -filter "full_name==R1"}
show_libs "get_libs NANGATE*   " {get_libs -quiet NANGATE*}

puts ""
puts "######## sta_case_insensitive_matching = 0 (restored) ########"
set sta_case_insensitive_matching 0
show "get_ports CLK1       " {get_ports -quiet CLK1}
show "get_ports clk1       " {get_ports -quiet clk1}
show "get_cells -filter ==R1" {get_cells -quiet -filter "full_name==R1"}
