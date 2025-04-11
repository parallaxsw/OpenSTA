proc try_expr {input} {
    puts "\[sta::_filter_expr_to_postfix \"$input\" 1]"
    puts [sta::_filter_expr_to_postfix $input 1]
}

proc try_error {input} {
    if {[catch {try_expr $input} error]} {
        puts $error
    } else {
        puts "no error raised"
    }
}

try_expr ""
try_expr "a"
try_expr "!a"
try_expr "a && b"
try_expr "a && !b"
try_expr "a || !(b && c)"
try_expr "!(a && b || c) && d || !(a || b && c)"
try_expr "!(a !~ z && b == y || c != x) && d || !(a || b && c)"
try_error "(a"
try_error "(a))"
try_error "a))))"
try_error "a + b"
