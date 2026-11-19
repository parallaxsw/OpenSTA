# Verify sta_cmd_unknown delegates to ::unknown in the caller's frame
# without re-evaluating ::unknown's return value as a Tcl script.

# Autoloaded proc returning a non-command value (e.g. a timezone name).
set ::auto_index(test_autoload_value) {
  proc ::test_autoload_value {} {
    return "US/Pacific"
  }
}
puts [test_autoload_value]

# Autoloaded proc with multi-word and brace-quoted args returning a
# command-like string that must not be executed a second time.
set ::auto_index(test_autoload_args) {
  proc ::test_autoload_args { a b } {
    return "puts [list $a $b]"
  }
}
puts [test_autoload_args "hello world" {$literal}]

# Autoloaded proc accessing a local variable in the caller's stack frame.
set ::auto_index(test_autoload_upvar) {
  proc ::test_autoload_upvar { var_name } {
    upvar 1 $var_name caller_var
    return "$caller_var in [uplevel 1 {namespace current}]"
  }
}
proc call_autoload_with_local {} {
  set caller_local 42
  return [test_autoload_upvar caller_local]
}
puts [call_autoload_with_local]

# Truly unknown command still raises an invalid command name error.
catch { nonexistent_sta_unknown_cmd arg1 } err
puts $err
