# get_db/set_db
read_liberty ../examples/nangate45_slow.lib.gz
read_verilog get_cell_hierarchy.v
link_design dut

create_clock -name clk1 -period 10 [get_ports clk1]
create_clock -name clk2 -period 20 [get_ports clk2]

puts "short name: [get_db program_short_name]"
puts "prog name: [get_db program_name]"
puts "version: [expr { [get_db program_version] != {} }]"

# Leaf vs hierarchical instances and pins.
puts "insts: [lsort [get_db insts u_blk*/blk_* .name]]"
puts "hinsts: [lsort [get_db hinsts u_blk* .name]]"
puts "hpins: [lsort [get_db hpins u_blk1/mid* .name]]"

# Patterns glob across the hierarchy separator.
puts "Q pins: [lsort [get_db pins u_blk1/blk_r?/Q .name]]"
puts "CK pins: [lsort [get_db pins */CK .name]]"
puts "hier nets: [lsort [get_db nets u_blk1/r* .name]]"
puts "top nets: [lsort [get_db nets w_mid* .name]]"
puts "ports: [lsort [get_db ports clk* .name]]"
puts "clocks: [lsort [get_db clocks clk* .name]]"
puts "lib cells: [lsort [get_db lib_cells *DFF_X1 .base_name]]"
puts "libs: [get_db libs * .name]"
puts "no match: '[get_db -quiet pins u_nope/*]'"

# .name is the hierarchical name, .base_name is the leaf name.
set r1 [get_db insts u_blk1/blk_r1]
puts "name: [get_db $r1 .name]"
puts "base_name: [get_db $r1 .base_name]"
puts "obj_type: [get_db $r1 .obj_type]"
puts "ref_name: [get_db $r1 .ref_name]"
puts "base_cell: [get_db $r1 .base_cell.base_name]"

# Attribute chains flatten at each step.
puts "r1 pins: [lsort [get_db $r1 .pins.name]]"
puts "r1 nets: [lsort [get_db $r1 .pins.net.name]]"
puts "blk1 kids: [lsort [get_db [get_db hinsts u_blk1] .insts.base_name]]"

set q [get_db pins u_blk1/blk_r1/Q]
puts "pin obj_type: [get_db $q .obj_type]"
puts "pin inst: [get_db $q .inst.name]"
puts "pin parent: [get_db $q .inst.parent.base_name]"
puts "pin port: [get_db $q .port.name]"
puts "pin base_pin: [get_db $q .base_pin.name]"

set r1q [get_db nets u_blk1/r1q]
puts "net inst: [get_db $r1q .inst.name]"
puts "net drivers: [lsort [get_db $r1q .drivers.name]]"

# -unique keeps one copy of each value.
puts "ref names: [lsort [get_db insts u_blk*/blk_* .ref_name -unique]]"
puts "unique objs: [llength [get_db [concat $q $q] -unique]]"
puts "clock sources: [lsort [get_db [get_clocks clk*] .sources.name]]"

# -if filtering, including chained attributes.
puts "seq insts: [lsort [get_db insts * -if {.is_sequential} .name]]"
puts "not seq: [lsort [get_db insts * -if {!.is_sequential} .name]]"
puts "buf or inv: [lsort [get_db insts * -if {.is_buffer || .is_inverter} .name]]"
puts "seq pins: [lsort [get_db pins */Q -if {.inst.is_sequential == true} .name]]"
puts "glob name: [lsort [get_db insts * -if {.name == *blk_r*} .base_name]]"
puts "ref match: [lsort [get_db insts * -if {.ref_name =~ DFF*} .base_name]]"
puts "parens: [lsort [get_db insts * \
                        -if {(.is_sequential || .is_buffer) && .ref_name !~ AND*} \
                        .base_name]]"
puts "quoted: [get_db insts * -if ".ref_name == \"BUF_X1\"" .base_name]"
puts "period: [lsort [get_db clocks * -if {.period >= 15} .name]]"

# set_db round trip.
set_db [get_db insts u_blk1/blk_*] .dont_touch true
puts "dont_touch: [get_db insts u_blk1/blk_r1 .dont_touch]"
puts "untouched: '[get_db insts u_blk2/blk_r3 .dont_touch]'"
set_db [get_db insts u_blk1/blk_r1] .dont_touch false
puts "dont_touch off: [get_db insts u_blk1/blk_r1 .dont_touch]"
set_db [get_db insts u_blk1/blk_r1] .my_attr hello
puts "inst attr: [get_db insts u_blk1/blk_r1 .my_attr]"
set_db $r1q .my_attr world
puts "net attr: [get_db nets u_blk1/r1q .my_attr]"

# all_fanout/all_fanin alias get_fanout/get_fanin, traversing the flat netlist.
puts "fanout: [lsort [get_db [all_fanout -from u_blk1/blk_r2/Q] .name]]"
puts "fanin: [lsort [get_db [all_fanin -to out -startpoints_only] .name]]"
puts "endpoints: [lsort [get_db [all_fanout -from u_blk1/blk_r2/Q -endpoints_only] .name]]"

# Results are Tcl lists, so llength and empty collections behave.
puts "llength: [llength [get_db insts u_blk*/blk_*]]"
puts "empty in: '[get_db [get_db -quiet insts no_such_inst] .name]'"

# Diagnostics.
get_db insts u_nope/*
set_db some_global_attr 3
puts "bad attr quiet: '[get_db -quiet insts u_blk1/blk_r1 .no_such_attr]'"
puts "bad attr: '[get_db insts u_blk1/blk_r1 .no_such_attr]'"
foreach bad {
  {get_db}
  {get_db .name}
  {get_db no_such_collection}
  {get_db insts u_blk1/blk_r1 no_dot}
  {get_db insts u_blk1/blk_r1 .}
  {get_db insts u_blk1/blk_r1 .pins..net}
  {get_db insts * -if {.is_buffer &&}}
  {get_db insts * -if {(.is_buffer}}
  {get_db insts * -if {.is_buffer .is_inverter}}
  {set_db [get_db insts u_blk1/blk_r1] no_dot 1}
  {set_db}
} {
  catch { uplevel #0 $bad } msg
  puts "error: $msg"
}
