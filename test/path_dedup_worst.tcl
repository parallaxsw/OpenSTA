read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd
read_sdc ../examples/gcd_sky130hd.sdc

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
    -dedup_by_word]

puts "resp_msg count: [count_pattern "resp_msg" $checks_rpt]"
puts "resp_msg[15] count: [count_pattern "resp_msg\\\[15\\\]" $checks_rpt]"
    -deduplication_mode keep_worst]

puts "resp_msg count: [count_pattern "resp_msg" $checks_rpt]\nresp_msg[15] count: [count_pattern "resp_msg\\\[15\\\]" $checks_rpt]"

file delete -force $checks_rpt
