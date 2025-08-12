read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog path_dedup_different.v
link_design esoteric_design

# SDC
set period 5
create_clock -period $period [get_ports CLK]
set clk_period_factor .2
set delay [expr $period * $clk_period_factor]
set_input_delay $delay -clock CLK {d[*]}
set_output_delay $delay -clock CLK [all_outputs]
set_input_transition .1 [all_inputs]

proc count_pattern {pattern filename} {
    set count 0
    set fh [open $filename r]
    while {[gets $fh line] >= 0} {
        if {[regexp $pattern $line]} {
            incr count
        }
    }
    close $fh
    return $count
}

proc make_checks_rpt {args} {
    set temp_file_id [file tempfile temp_filename]
    close $temp_file_id
    
    report_checks\
        -format end\
        {*}$args > $temp_filename
        
    return $temp_filename
}

set checks_rpt [make_checks_rpt\
    -group_path_count 999\
    -fields {input_pins slew}\
    -deduplication_mode keep_different]

# Only store[1] is different, the rest should be identical and thus any of them
# can be the other one.
puts "store count: [count_pattern "store" $checks_rpt]"
puts "store[1] count: [count_pattern "store\\\[1\\\]" $checks_rpt]"

file delete -force $checks_rpt
