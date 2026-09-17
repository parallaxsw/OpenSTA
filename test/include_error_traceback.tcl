# The traceback for a command that fails while reading a file.
set sta_error_traceback 1
set sta_continue_on_error 1

proc traceback_inner { x } {
  return [expr { $x / 0 }]
}

proc traceback_outer { } {
  traceback_inner 3
}

traceback_outer
