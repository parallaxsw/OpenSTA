# report_checks -format summary field reporting
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

puts "--- default summary ---"
report_checks -group_path_count 3 -format summary

puts "--- built-in summary fields ---"
report_checks -group_path_count 3 -format summary \
  -fields {group startpoint_clock endpoint_clock required arrival crpr}

puts "--- pin metrics in summary ---"
report_checks -group_path_count 3 -format summary \
  -fields {capacitance slew fanout}

puts "--- custom path attr field in summary ---"
define_property -object_type instance -type string orig_name
set_property [get_cells r1] orig_name "rtl_r1"
set_property [get_cells r2] orig_name "rtl_r2"
set_property [get_cells r3] orig_name "rtl_r3"

sta::make_report_path_attr_field orig_name 24
sta::set_report_path_field_properties orig_name "Orig Name" 24 1

report_checks -group_path_count 3 -format summary -fields {orig_name}
report_checks -group_path_count 3 -format summary -fields {endpoint_orig_name}

puts "--- custom summary attr field + field properties ---"
define_property -object_type instance -type string stage
set_property [get_cells r2] stage "s0"
set_property [get_cells r3] stage "s1"
sta::make_report_summary_attr_field end_stage 0 stage 10
sta::set_report_summary_field_properties end_stage "End Stage" 10 1
report_checks -group_path_count 3 -format summary -fields {end_stage}

puts "--- custom summary field order ---"
sta::set_report_summary_field_order {startpoint startpoint_orig_name endpoint endpoint_orig_name slack}
sta::set_report_path_format summary
sta::report_path_ends [find_timing_paths -group_path_count 3]
