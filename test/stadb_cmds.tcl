# Tcl command contract for write_sta_db / read_sta_db.

source stadb_helpers.tcl

puts "cmd write_sta_db $sta::cmd_args(write_sta_db)"
puts "cmd read_sta_db $sta::cmd_args(read_sta_db)"

help write_sta_db
help read_sta_db

if { [catch { write_sta_db } msg] } {
  puts "cmd write_sta_db argc: $msg"
}
if { [catch { write_sta_db -no_compress a b } msg] } {
  puts "cmd write_sta_db extra: $msg"
}
if { [catch { read_sta_db } msg] } {
  puts "cmd read_sta_db argc: $msg"
}
if { [catch { read_sta_db /no/such/stadb.file } msg] } {
  puts "cmd read_sta_db missing: $msg"
}

read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
sta::find_timing -full_update

set raw [make_result_file "stadb_cmds.raw.stadb"]
set gz [make_result_file "stadb_cmds.gz.stadb"]
write_sta_db -no_compress $raw
write_sta_db $gz
puts "cmd uncompressed larger: [expr { [file size $raw] > [file size $gz] }]"

set cold [stadb_run "read_sta_db $raw
report_checks -digits 4 -path_delay min_max" cmds_raw]
set warm [stadb_run "read_sta_db $gz
report_checks -digits 4 -path_delay min_max" cmds_gz]
stadb_check "cmd compress roundtrip" $cold $warm
