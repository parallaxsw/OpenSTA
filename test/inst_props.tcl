read_liberty gf180mcu_sram.lib.gz
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog inst_props.v
link inst_props

proc test_property {property} {
    puts "== $property =="
    set property_is_inverted [string match "!*" $property]
    set objects [get_cells -filter $property]
    set object_names []
    foreach object $objects {
        lappend object_names [get_full_name $object]
    }
    foreach name [lsort $object_names] {
        puts "$name"
    }
    puts "===="
}

test_property is_memory
test_property design_type==module
test_property is_buffer
