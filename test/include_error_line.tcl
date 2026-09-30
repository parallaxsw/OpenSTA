# An error in a multi-line command is reported at its first line.
catch { read_sdc include_error_line.sdc } msg
puts $msg
