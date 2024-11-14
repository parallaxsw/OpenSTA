# Tests whether the has_memory attribute works for cells and libcells
read_liberty ../examples/gf180mcu_sram.lib.gz
read_liberty asap7_small.lib.gz
read_verilog get_has_memory.v
link get_has_memory

# Test that the has_memory attribute is set correctly for cells
puts {[get_cells -filter has_memory]}
report_object_full_names [get_cells -filter has_memory]
puts {[get_lib_cells -filter has_memory]}
report_object_full_names [get_lib_cells -filter has_memory]
