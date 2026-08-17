# Inventory of get_property names plus a cold/warm property dump.
# A new property == "..." in search/Property.cc fails this golden.

source stadb_helpers.tcl

array set stadb_prop_type {
  Library library
  LibertyLibrary liberty_library
  Cell cell
  LibertyCell liberty_cell
  Port port
  LibertyPort liberty_port
  Instance instance
  Pin pin
  Net net
  Clock clock
  Scene scene
  Mode mode
}

puts "# serialize a new property in the matching stadb writer, or mark dropped"
puts "object\tproperty"

set prop_file [stadb_repo_file .. search Property.cc]
set stream [open $prop_file "r"]
set text [read $stream]
close $stream

set current ""
set seen {}
foreach line [split $text "\n"] {
  if { [string match "*Properties::getProperty*" $line] \
         && [regexp {([A-Za-z]+) \*} $line -> ctype] } {
    set current $ctype
  }
  if { $current eq "" || $current eq "PathEnd" } {
    continue
  }
  if { ![info exists stadb_prop_type($current)] } {
    continue
  }
  set otype $stadb_prop_type($current)
  foreach {all name} [regexp -all -inline {property == "([^"]+)"} $line] {
    set key "$otype $name"
    if { [lsearch -exact $seen $key] < 0 } {
      lappend seen $key
      puts "$otype\t$name"
    }
  }
}

puts "# get_db roots"
set extras [stadb_repo_file .. tcl Extras.tcl]
set stream [open $extras "r"]
set extras_text [read $stream]
close $stream
set in_roots 0
foreach line [split $extras_text "\n"] {
  if { [string match "*array set get_db_roots *" $line] } {
    set in_roots 1
    continue
  }
  if { $in_roots } {
    if { [regexp {^  ([A-Za-z_]+)[ \t]+} $line -> root] } {
      puts "get_db_root\t$root"
    } elseif { [string match "array set *" [string trim $line]] \
                 || [string match "#*" [string trim $line]] } {
      set in_roots 0
    }
  }
}

# Round-trip dump of scalar properties on example1.
set dump {
  foreach clk [get_clocks *] {
    stadb_dump_clock $clk
  }
  foreach cell {BUF_X1 DFF_X1 AND2_X1 CLKGATE_X1} {
    set c [get_lib_cells */$cell]
    stadb_dump_liberty_cell $c
  }
  foreach inst [get_cells *] {
    stadb_dump_instance $inst
  }
  foreach pin [get_pins *] {
    stadb_dump_pin $pin
  }
}

set build {read_liberty ../examples/nangate45_slow.lib.gz
read_verilog ../examples/example1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
sta::find_timing -full_update}

set db [make_result_file "stadb_prop.stadb"]
stadb_run "$build
write_sta_db $db" prop_w
set cold [stadb_run "$build
$dump" prop_c]
set warm [stadb_run "read_sta_db $db
$dump" prop_r]
stadb_check "prop dump" $cold $warm
