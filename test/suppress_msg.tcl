# suppress and unsuppress message ids
proc cmd { msg } {
  puts "call $msg"
  sta::sta_warn 1 "cmd error"
  sta::sta_error 2 "cmd error"
  puts "after error"
}

catch { cmd 1 } error
puts $error

suppress_msg 1
suppress_msg 2
catch { cmd 2 } error
puts "caught $error"

set sta_continue_on_error 1
cmd 3
cmd 4
