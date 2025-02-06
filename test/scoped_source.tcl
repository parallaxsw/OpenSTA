set test_string foo

proc source_file {} {
    set test_string bar
    source ./scoped_sourceable.tcl    
}

source_file

set ::sta_scoped_source 1

source_file
