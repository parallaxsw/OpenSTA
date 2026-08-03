# read_sdc must accept gzipped SDC via libz, even when Tcl has no zlib command
# (packaged Tcl 8.6 builds used by silisizer have hit this; MOH-91).

source helpers.tcl

read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top

set sdc_file [make_result_file "read_sdc_gzip.sdc"]
set stream [open $sdc_file w]
puts $stream {create_clock -name clk -period 10 {clk1 clk2 clk3}}
puts $stream {set_input_delay -clock clk 0 {in1 in2}}
close $stream
exec gzip -f $sdc_file
set sdc_gz "${sdc_file}.gz"

# Simulate packaged Tcl without the zlib command.
set zlib_cmd [namespace which -command ::zlib]
if { $zlib_cmd != "" } {
  rename ::zlib ::zlib_sta_test_saved
}

if { [catch {read_sdc $sdc_gz} err] } {
  if { $zlib_cmd != "" } {
    rename ::zlib_sta_test_saved ::zlib
  }
  error $err
}

if { $zlib_cmd != "" } {
  rename ::zlib_sta_test_saved ::zlib
}

set clocks [get_clocks]
if { [llength $clocks] != 1 } {
  error "expected 1 clock after read_sdc of gzipped SDC, got [llength $clocks]"
}
puts "clocks [get_property [lindex $clocks 0] name] [get_property [lindex $clocks 0] period]"

# Truncated/corrupt gzip must error during decompress, not apply a partial SDC.
set sdc_trunc [make_result_file "read_sdc_gzip_trunc.sdc"]
set stream [open $sdc_trunc w]
puts $stream {create_clock -name clk_trunc -period 5 {clk1}}
# Pad so the compressed stream is large enough to truncate mid-payload.
for {set i 0} {$i < 200} {incr i} {
  puts $stream {set_input_delay -clock clk_trunc 0 {in1 in2}}
}
close $stream
exec gzip -f $sdc_trunc
set trunc_gz "${sdc_trunc}.gz"
set size [file size $trunc_gz]
set keep [expr { $size / 2 }]
if { $keep < 20 } {
  set keep 20
}
set fin [open $trunc_gz r]
fconfigure $fin -translation binary
set data [read $fin $keep]
close $fin
set fout [open $trunc_gz w]
fconfigure $fout -translation binary
puts -nonewline $fout $data
close $fout

if { ![catch {read_sdc $trunc_gz}] } {
  error "expected read_sdc of truncated gzip SDC to fail"
}
# ungzip fails before include_file runs, so clk_trunc must not appear.
foreach clk [get_clocks] {
  if { [get_property $clk name] eq "clk_trunc" } {
    error "truncated gzip must not apply partial SDC constraints"
  }
}
puts "truncated_gzip_rejected"
