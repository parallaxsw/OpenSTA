read_liberty gf180mcu_sram.lib.gz
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog inst_props.v
link inst_props

proc test_property {property} {
    puts "== $property =="
    set property_is_inverted [string match "!*" $property]
    set objects [get_pins -filter $property]
    set object_names []
    foreach object $objects {
        if { [$object [string map {! ""} $property]] == "$property_is_inverted" } {
            puts "$property method and $property property returning different values"
        }
        lappend object_names [get_full_name $object]
    }
    foreach name [lsort $object_names] {
        puts "$name"
    }
    puts "===="
}

test_property is_clock
test_property is_register_clock
test_property is_rise_edge_triggered
test_property is_fall_edge_triggered
