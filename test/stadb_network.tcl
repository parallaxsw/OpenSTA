# Network: hierarchy, buses, instance attributes, user properties.

source stadb_helpers.tcl

set hier_file [make_result_file "stadb_network.hier.stadb"]
set hier_cold_v [make_result_file "stadb_network.hier.cold.v"]
set hier_warm_v [make_result_file "stadb_network.hier.warm.v"]
set hier_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog get_cell_hierarchy.v
link_design dut
create_clock -name clk -period 10 {clk1 clk2 clk3}}
set hier_dump {foreach inst [get_cells -hierarchical *] {
  stadb_dump_instance $inst
}
foreach pin [get_pins -hierarchical *] {
  stadb_dump_pin $pin
}}
stadb_run "$hier_build
sta::find_timing -full_update
write_sta_db $hier_file" net_hw
set cold_h [stadb_run "$hier_build
$hier_dump
write_verilog $hier_cold_v" net_hc]
set warm_h [stadb_run "read_sta_db $hier_file
$hier_dump
write_verilog $hier_warm_v" net_hr]
stadb_check "network hierarchy" $cold_h $warm_h
stadb_check_files "network hierarchy verilog" $hier_cold_v $hier_warm_v

set bus_file [make_result_file "stadb_network.bus.stadb"]
set bus_cold_v [make_result_file "stadb_network.bus.cold.v"]
set bus_warm_v [make_result_file "stadb_network.bus.warm.v"]
set bus_build {read_liberty stadb_kitchen.lib
read_verilog stadb_kitchen.v
link_design top
create_clock -name clk -period 10 clk}
stadb_run "$bus_build
write_sta_db $bus_file" net_bw
stadb_run "$bus_build
write_verilog $bus_cold_v" net_bc
stadb_run "read_sta_db $bus_file
write_verilog $bus_warm_v" net_br
stadb_check_files "network buses" $bus_cold_v $bus_warm_v

set attrs_file [make_result_file "stadb_network.attrs.stadb"]
set attrs_build {read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog verilog_attribute.v
link_design counter
create_clock -name clk -period 10 clk}
set attrs_dump {report_checks -format json -digits 4}
stadb_run "$attrs_build
sta::find_timing -full_update
write_sta_db $attrs_file" net_aw
set cold_a [stadb_run "$attrs_build
$attrs_dump" net_ac]
set warm_a [stadb_run "read_sta_db $attrs_file
$attrs_dump" net_ar]
stadb_check "network verilog_src" $cold_a $warm_a
puts "network verilog_src present: [regexp {\"verilog_src\": \"[^\"]} $cold_a]"

set user_file [make_result_file "stadb_network.user.stadb"]
set user_build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
define_property -object_type pin -type string owner
set_property [get_pins u1/Z] owner alice
define_property -object_type instance -type bool crit
set_property [get_cells u2] crit true}
set user_dump {puts "network user pin owner [stadb_prop [get_pins u1/Z] owner]"
puts "network user inst crit [stadb_prop [get_cells u2] crit]"}
stadb_run "$user_build
write_sta_db $user_file" net_uw
set cold_u [stadb_run "$user_build
$user_dump" net_uc]
set warm_u [stadb_run "read_sta_db $user_file
define_property -object_type pin -type string owner
define_property -object_type instance -type bool crit
$user_dump" net_ur]
# User properties are not serialized. Golden the explicit drop.
puts "network user_properties status: [expr {
  $cold_u eq $warm_u ? "restored" : "dropped"}]"
if { $cold_u ne $warm_u } {
  puts "  fix: not serialized (stadb network/sdc)"
  puts "  cold: [string map [list \n { / }] $cold_u]"
  puts "  warm: [string map [list \n { / }] $warm_u]"
}
