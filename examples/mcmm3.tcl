# mmcm reg1 parasitics
read_liberty asap7_small_ff.lib.gz
read_liberty asap7_small_ss.lib.gz
read_verilog reg1_asap7.v
link_design top

read_sdc -mode mode1 mcmm2_mode1.sdc
read_sdc -mode mode2 mcmm2_mode2.sdc

read_spef -name reg1_ff reg1_asap7.spef
read_spef -name reg1_ss reg1_asap7_ss.spef

define_scene scene1 -mode mode1 -liberty asap7_small_ff -spef reg1_ff
define_scene scene2 -mode mode2 -liberty asap7_small_ss -spef reg1_ss

report_checks -scenes scene1
report_checks -scenes scene2
report_checks -group_path_count 4

# scenes/modes cross to Tcl as objects; mode/scene counts stay 2.
puts "--- scene/mode object coverage ---"
puts "num modes         : [llength [get_modes *]]"
puts "num scenes        : [llength [get_scenes]]"
puts "cmd_scene type    : [sta::object_type [sta::cmd_scene]]"
puts "cmd_mode_name     : [sta::cmd_mode_name]"
foreach mode [get_modes *] {
  puts "get_modes type    : [sta::object_type $mode]"
}
foreach scene [get_scenes] {
  puts "get_scenes type   : [sta::object_type $scene]"
}
set_scene scene2
puts "set_scene scene2 -> cmd_mode_name : [sta::cmd_mode_name]"
set_scene scene1
puts "set_scene scene1 -> cmd_mode_name : [sta::cmd_mode_name]"
# scene objects (not just names) flow through report_checks.
report_checks -scenes [get_scenes scene2]
