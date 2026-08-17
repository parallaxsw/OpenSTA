# Constraint-mode aliases on OpenSTA modes/scenes.
proc object_names { objects } {
  set names {}
  foreach_in_collection obj $objects {
    lappend names [get_name $obj]
  }
  return [lsort $names]
}

read_liberty ../examples/asap7_small_ff.lib.gz
read_verilog ../examples/reg1_asap7.v
link_design top

puts "default interactive: [get_interactive_constraint_modes]"
puts "default modes: [object_names [get_modes *]]"
puts "default obj_type: [get_db constraint_modes * .obj_type]"
puts "default is_active: [get_property [get_modes default] is_active]"
puts "default is_dynamic: [get_property [get_modes default] is_dynamic]"
# No pattern, -if then .name.
puts "default get_db active: [get_db [get_db constraint_modes -if {.is_active && !.is_dynamic}] .name]"

# Select a named mode, then apply SDC.
set_interactive_constraint_modes turbo_ssg_m40
puts "after set interactive: [get_interactive_constraint_modes]"
puts "cmd mode: [sta::cmd_mode_name]"
create_clock -name turbo_clk -period 10 clk1
puts "turbo clocks: [object_names [get_clocks -quiet *]]"

foreach cm [get_interactive_constraint_modes] {
  if { [regexp -nocase "turbo_ssg" $cm] } {
    puts "matched turbo_ssg for $cm"
  }
}

# SDC is isolated per mode.
set_mode mode2
puts "mode2 clocks before create: [object_names [get_clocks -quiet *]]"
create_clock -name mode2_clk -period 20 clk1
puts "mode2 clocks: [object_names [get_clocks -quiet *]]"
puts "mode2 is_active: [get_property [get_modes mode2] is_active]"
puts "turbo is_active: [get_property [get_modes turbo_ssg_m40] is_active]"

set_mode turbo_ssg_m40
puts "turbo clocks again: [object_names [get_clocks -quiet *]]"

puts "all mode names: [object_names [get_modes *]]"
puts "active name: [get_db [get_db constraint_modes -if {.is_active}] .name]"
puts "dynamic names: '[get_db [get_db constraint_modes -if {.is_dynamic}] .name]'"
puts "get_modes -filter active: [object_names [get_modes -filter {is_active == true}]]"

puts "all_constraint_modes: [object_names [all_constraint_modes]]"
puts "all_constraint_modes -active: [object_names [all_constraint_modes -active]]"
puts "all_constraint_modes -type static: [object_names [all_constraint_modes -type static]]"
puts "all_constraint_modes -type dynamic: '[all_constraint_modes -type dynamic]'"
if { [catch { all_constraint_modes -type bogus } msg] } {
  puts "all_constraint_modes bad type: $msg"
}

# Multiple interactive names: SDC still writes to the first.
set_interactive_constraint_modes {mode2 turbo_ssg_m40}
puts "multi interactive: [get_interactive_constraint_modes]"
puts "multi cmd mode: [sta::cmd_mode_name]"

# Empty list (`set_interactive_constraint_modes { }`) follows cmd_mode.
set_interactive_constraint_modes { }
puts "cleared interactive: [get_interactive_constraint_modes]"

# Singular alias and object arguments.
set_interactive_constraint_modes [get_modes turbo_ssg_m40]
puts "from object: [get_interactive_constraint_mode]"

# Scenes / analysis views. define_scene selects the new scene (and its mode).
read_spef -name reg1_ff ../examples/reg1_asap7.spef
define_scene scene_turbo -mode turbo_ssg_m40 -liberty asap7_small_ff -spef reg1_ff
puts "scene_turbo is_active: [get_property [get_scenes scene_turbo] is_active]"
puts "after define_scene turbo cmd mode: [sta::cmd_mode_name]"
define_scene scene_mode2 -mode mode2 -liberty asap7_small_ff -spef reg1_ff
puts "scenes: [object_names [get_scenes *]]"
puts "views: [lsort [get_db analysis_views * .name]]"
puts "scene_mode2 is_active: [get_property [get_scenes scene_mode2] is_active]"
puts "after define_scene mode2 cmd mode: [sta::cmd_mode_name]"
set_scene scene_turbo
puts "after set_scene active view: [get_db [get_db analysis_views -if {.is_active}] .name]"
puts "after set_scene cmd mode: [sta::cmd_mode_name]"
puts "after set_scene turbo is_active: [get_property [get_modes turbo_ssg_m40] is_active]"
puts "after set_scene mode2 is_active: [get_property [get_modes mode2] is_active]"
