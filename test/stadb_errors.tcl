# Error paths and unsupported sessions.

source stadb_helpers.tcl

if { [catch { read_sta_db /no/such/stadb.file } msg] } {
  puts "error missing: [stadb_scrub $msg]"
}

set junk [make_result_file "stadb_errors.junk.stadb"]
set stream [open $junk "w"]
puts $stream "not a stadb"
close $stream
if { [catch { read_sta_db $junk } msg] } {
  puts "error magic: [stadb_scrub $msg]"
}

read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
sta::find_timing -full_update
set good [make_result_file "stadb_errors.good.stadb"]
write_sta_db $good

set trunc [make_result_file "stadb_errors.trunc.stadb"]
set stream [open $good "rb"]
set bytes [read $stream 32]
close $stream
set stream [open $trunc "wb"]
puts -nonewline $stream $bytes
close $stream
if { [catch { read_sta_db $trunc } msg] } {
  puts "error truncated: [stadb_scrub $msg]"
}

# Corrupt the checksum / payload after a valid header by flipping a later byte.
set corrupt [make_result_file "stadb_errors.corrupt.stadb"]
set stream [open $good "rb"]
set data [read $stream]
close $stream
set last [expr { [string length $data] - 1 }]
set flipped [format %c [expr { [scan [string index $data $last] %c] ^ 0xff }]]
set data [string replace $data $last $last $flipped]
set stream [open $corrupt "wb"]
puts -nonewline $stream $data
close $stream
if { [catch { read_sta_db $corrupt } msg] } {
  puts "error checksum: [stadb_scrub $msg]"
}

if { [catch { read_sta_db $junk } msg] } {
  puts "error magic after design: [stadb_scrub $msg]"
  puts "error magic keeps cells: [llength [stadb_names [get_cells *]]]"
}

# POCV graph cannot be stored.
set pocv [make_result_file "stadb_errors.pocv.stadb"]
set pocv_out [stadb_run "read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
sta::find_timing -full_update
set sta_pocv_mode normal
if { \[catch { write_sta_db $pocv } msg\] } {
  puts \$msg
}" err_pocv]
puts "error pocv: [stadb_scrub $pocv_out]"

# Multi-scene cannot be stored.
set mcmm [make_result_file "stadb_errors.mcmm.stadb"]
set mcmm_out [stadb_run "read_liberty ../examples/asap7_small_ff.lib.gz
read_liberty ../examples/asap7_small_ss.lib.gz
read_verilog ../examples/reg1_asap7.v
link_design top
read_sdc -mode mode1 ../examples/mcmm2_mode1.sdc
read_sdc -mode mode2 ../examples/mcmm2_mode2.sdc
define_scene scene1 -mode mode1 -liberty asap7_small_ff
define_scene scene2 -mode mode2 -liberty asap7_small_ss
if { \[catch { write_sta_db $mcmm } msg\] } {
  puts \$msg
}" err_mcmm]
puts "error scenes: [stadb_scrub $mcmm_out]"

# CCS is dropped with warning 2741, write still succeeds.
set ccs [make_result_file "stadb_errors.ccs.stadb"]
set ccs_out [stadb_run "read_liberty asap7_ccsn.lib.gz
write_sta_db $ccs" err_ccs]
puts "error ccs: [stadb_scrub $ccs_out]"
puts "error ccs wrote: [file exists $ccs]"
