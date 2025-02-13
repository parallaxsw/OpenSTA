set test_string foo

proc source_file {} {
    set test_string bar
    read_cmds ./scoped_sourceable.tcl
    source ./scoped_sourceable.tcl    
}

source_file
