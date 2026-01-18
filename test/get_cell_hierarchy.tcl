# hierarchical pattern matching with u_blk1/blk*
read_liberty ../examples/nangate45_slow.lib.gz
read_verilog get_cell_hierarchy.v
link_design dut

puts {[get_cells -hier u_blk1/blk*]}
report_object_full_names [get_cells -hier u_blk1/blk*]

puts {[get_cells -hier u_blk*]}
report_object_full_names [get_cells -hier u_blk*]

puts {[get_cells u_blk1/blk_r1]}
report_object_full_names [get_cells u_blk1/blk_r1]

puts {[get_cells -hier *r*]}
report_object_full_names [get_cells -hier *r*]