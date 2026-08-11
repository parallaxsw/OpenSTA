# Instance attributes survive a .stadb round trip.
#
# Separate from the stadb test because that design carries no attributes:
# nangate45 example1 is hand written, while these come from a synthesizer that
# tags every instance with the source line it came from. report_json exposes
# the "src" attribute as verilog_src, which is what a debug flow follows back
# to the rtl, so losing it on a cache hit would be silent and annoying.
#
# Restoring runs in a child process so the attributes cannot come from this
# session's already linked netlist.

set stadb_app [info nameofexecutable]
set stadb_tmp "stadb_attrs_tmp[pid]"

proc stadb_attrs_run { body } {
  global stadb_app stadb_tmp
  set script "$stadb_tmp.tcl"
  set stream [open $script "w"]
  puts $stream $body
  puts $stream "exit"
  close $stream
  set failed [catch {
    exec $stadb_app -no_init -no_splash -exit $script 2>@1
  } output]
  file delete -force $script
  if { $failed } {
    return "child failed: $output"
  }
  return $output
}

set stadb_report "report_checks -format json -digits 4"
set stadb_build "read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog verilog_attribute.v
link_design counter
create_clock -name clk -period 10 clk"

eval $stadb_build
sta::find_timing -full_update
write_sta_db "$stadb_tmp.stadb"

# The whole json report, not just the attribute, since a restore that dropped
# an instance would also change verilog_src without the attribute being at
# fault.
set cold [stadb_attrs_run "$stadb_build\n$stadb_report"]
set warm [stadb_attrs_run "read_sta_db $stadb_tmp.stadb\n$stadb_report"]
puts "json matches baseline: [expr { $cold == $warm }]"
if { $cold != $warm } {
  puts "--- baseline ---\n$cold"
  puts "--- restored ---\n$warm"
}

# Guard against the comparison above passing because both sides are empty.
set attrs 0
foreach line [split $cold "\n"] {
  if { [regexp {"verilog_src": "counter|"verilog_src": "synthesis} $line] } {
    incr attrs
  }
}
puts "baseline reports source attributes: [expr { $attrs > 0 }]"

file delete -force "$stadb_tmp.stadb"
