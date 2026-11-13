# report_checks -format summary fields
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

# Default fields.
report_checks -group_path_count 3 -format summary
report_checks -group_path_count 3 -format summary -digits 4

define_property -object_type instance -type string orig_name
set_property [get_cells r1] orig_name "rtl_r1"
set_property [get_cells r2] orig_name "rtl_r2"
set_property [get_cells r3] orig_name "rtl_r3"

# A path field is reported for the path startpoint and endpoint.
sta::make_report_path_attr_field orig_name 24
report_checks -group_path_count 3 -format summary -fields {orig_name}
report_checks -group_path_count 3 -format summary -fields {endpoint_orig_name}

# Fields added with make_report_summary_attr_field.
sta::make_report_summary_attr_field start_inst orig_name 1 16
sta::make_report_summary_attr_field end_inst orig_name 0 16
report_checks -group_path_count 3 -format summary -fields {start_inst end_inst}

# Fields do not carry over to the next report.
report_checks -group_path_count 3 -format summary
