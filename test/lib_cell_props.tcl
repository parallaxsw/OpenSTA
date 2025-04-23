read_liberty ../examples/sky130hd_tt.lib.gz
read_liberty gf180mcu_sram.lib.gz

proc test_property {property} {
    puts "== $property =="
    set property_is_inverted [string match "!*" $property]
    set objects [get_lib_cell -filter $property]
    set object_names []
    foreach object $objects {
        if { [$object [string map {! ""} $property]] == "$property_is_inverted" } {
            puts "$property method and $property property returning different values"
        }
        lappend object_names "[get_full_name $object]"
    }
    foreach name [lsort $object_names] {
        puts "$name"
    }
    puts "===="
}

set_dont_use sky130_fd_sc_hd__sdf*

test_property !has_timing_model
test_property is_integrated_clock_gating_cell
test_property is_sequential
test_property dont_use
test_property is_memory
test_property is_physical_only
