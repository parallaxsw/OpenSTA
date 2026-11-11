# report_checks -format summary with custom attribute fields
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

define_property -object_type instance -type string orig_name
set_property [get_cells r1] orig_name "rtl_r1"
set_property [get_cells r2] orig_name "rtl_r2"
set_property [get_cells r3] orig_name "rtl_r3"

sta::make_report_path_attr_field orig_name 24
sta::set_report_path_field_properties orig_name "Orig Name" 24 1

report_checks -group_path_count 3 -format summary -fields {orig_name}
report_checks -group_path_count 3 -format summary
