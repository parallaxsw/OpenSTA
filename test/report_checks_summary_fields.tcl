# report_checks -format summary fields
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

# Default fields.
report_checks -group_path_count 3 -format summary
report_checks -group_path_count 3 -format summary -digits 4

# Fields are reported for the path startpoint and endpoint.
report_checks -group_path_count 3 -format summary -fields {capacitance slew fanout}

define_property -object_type instance -type string src
set_property [get_cells r2] src "example1.v:7"
set_property [get_cells r3] src "example1.v:10"
report_checks -group_path_count 3 -format summary -fields {src_attr}

define_property -object_type instance -type string orig_name
set_property [get_cells r1] orig_name "rtl_r1"
set_property [get_cells r2] orig_name "rtl_r2"
set_property [get_cells r3] orig_name "rtl_r3"
sta::make_report_path_attr_field orig_name 24
report_checks -group_path_count 3 -format summary -fields {orig_name}

# Fields do not carry over to the next report.
report_checks -group_path_count 3 -format summary
