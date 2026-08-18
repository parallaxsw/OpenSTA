# Tcl command contract for write_lib_db / read_lib_db.

source stadb_helpers.tcl

puts "cmd write_lib_db $sta::cmd_args(write_lib_db)"
puts "cmd read_lib_db $sta::cmd_args(read_lib_db)"

help write_lib_db
help read_lib_db

if { [catch { write_lib_db } msg] } {
  puts "cmd write_lib_db argc: $msg"
}
if { [catch { write_lib_db a } msg] } {
  puts "cmd write_lib_db one: $msg"
}
if { [catch { write_lib_db a b c } msg] } {
  puts "cmd write_lib_db extra: $msg"
}
if { [catch { read_lib_db } msg] } {
  puts "cmd read_lib_db argc: $msg"
}
if { [catch { read_lib_db a b } msg] } {
  puts "cmd read_lib_db extra: $msg"
}

set bad_ext [make_result_file "lib_db_cmds.bad.txt"]
if { [catch { read_lib_db $bad_ext } msg] } {
  puts "cmd read_lib_db ext: [stadb_scrub $msg]"
}

set missing [make_result_file "lib_db_cmds.missing.libdb"]
if { [catch { read_lib_db $missing } msg] } {
  puts "cmd read_lib_db missing: [stadb_scrub $msg]"
}

read_liberty ../examples/nangate45_slow.lib.gz
if { [catch { write_lib_db [get_libs *] $bad_ext } msg] } {
  puts "cmd write_lib_db ext: [stadb_scrub $msg]"
}
