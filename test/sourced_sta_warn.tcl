proc deepest {error} {
    if { $error } {
        ::sta::sta_error 3 "and this one? 19"
    } else {
        ::sta::sta_warn 2 "the line number here better be 17"
    }
}

proc deeper {error} {
    deepest $error
}

proc deep {error} {
    deeper $error
}

deep 0

catch {deep 1} error
puts $error
