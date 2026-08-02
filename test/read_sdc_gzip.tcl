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
