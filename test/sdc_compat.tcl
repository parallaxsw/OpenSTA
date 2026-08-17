# SDC compatibility helpers used by constraint scripts.
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd
create_clock -name clk -period 10 [get_ports clk]

# alias name definition
alias my_echo puts
my_echo "alias ok"

# redirect /dev/null {script}
redirect /dev/null {
  set RFLATCH [get_cells -quiet -hierarchical {*xtRF*latchout*}]
}
puts "RFLATCH size: [sizeof_collection $RFLATCH]"
if {[sizeof_collection $RFLATCH] != 0} {
  puts "setting multicycle paths"
}

# get_ports glob with [*] in the middle used to throw std::stoi.
if { [catch { get_ports -quiet {bus[*]*tail[*]} } gports] } {
  puts "glob bus error: $gports"
} else {
  puts "glob bus count: [sizeof_collection $gports]"
}
puts "req_msg bits: [sizeof_collection [get_ports req_msg[*]]]"

# get_db pins .name <pattern>
puts "clk pin count: [llength [get_db pins .name */CLK]]"

# Clock input ports:
#   get_ports [get_db ports -if {.is_clock_used_as_clock && .direction == in}]
puts "clock in ports: [lsort [get_object_name [get_db ports -if {.is_clock_used_as_clock && .direction == in}]]]"
puts "clk1 is_clock_used_as_clock: [get_property [get_ports clk] is_clock_used_as_clock]"
puts "req_val is_clock_used_as_clock: [get_property [get_ports req_val] is_clock_used_as_clock]"

# case_value from set_case_analysis
set dpin [index_collection [get_pins -hierarchical */D] 0]
set qpin [index_collection [get_pins -hierarchical */Q] 0]
set_case_analysis 1 $dpin
puts "case set: [get_property $dpin case_value]"
puts "case db: [get_db $dpin .case_value]"
puts "case unset: '[get_property $qpin case_value]'"

# get_fanin on an empty collection must not abort.
set empty_fanin [get_fanin -to [get_pins -quiet -hierarchical *no_such_pin*]]
puts "empty fanin: [sizeof_collection $empty_fanin]"
