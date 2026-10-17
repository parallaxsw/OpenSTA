source helpers.tcl
read_liberty asap7_small.lib.gz

# Create the aliases through edits: reading assigns would merge their names.
set verilog_file [make_result_file verilog_input_alias.v]
set stream [open $verilog_file w]
puts $stream {module alias_top(external_input, other_input, external_output, external_io);
  input external_input, other_input;
  output external_output;
  inout external_io;
  wire internal_input, internal_output, internal_io;
  supply1 power_net;
  supply0 ground_net;
  BUFx2_ASAP7_75t_R buffer (.A(internal_input), .Y(internal_output));
  BUFx2_ASAP7_75t_R io_buffer (.A(internal_io), .Y());
endmodule}
close $stream

proc load_input_alias {} {
  global verilog_file
  read_verilog $verilog_file
  link_design alias_top
  foreach {net port} {internal_input external_input
                     internal_output external_output
                     internal_io external_io} {
    if {![connect_pin $net $port]} {
      error "could not connect $port to $net"
    }
  }
}

proc input_alias_file_bytes {filename} {
  set stream [open $filename rb]
  set bytes [read $stream]
  close $stream
  return $bytes
}

load_input_alias
set written_file [make_result_file verilog_input_alias_written.v]
write_verilog $written_file
set text [input_alias_file_bytes $written_file]
foreach alias {"assign internal_input = external_input;"
               "assign external_output = internal_output;"
               "assign external_io = internal_io;"} {
  if {[string first $alias $text] < 0} {
    error "missing or reversed alias: $alias"
  }
}
if {[string first "assign internal_io = external_io;" $text] >= 0} {
  error "inout alias changed direction"
}
puts "input/output/inout aliases: correct directions"

read_verilog $written_file
link_design alias_top
set starts [get_fanin -to [get_ports external_output] -startpoints_only]
if {[llength $starts] != 1
    || [get_full_name [lindex $starts 0]] ne "external_input"} {
  error "input connectivity lost after Verilog round trip"
}
puts "renamed input round trip: connected"

# Check both a distinct internal name and a net named after one input.
foreach net {internal_input external_input} {
  load_input_alias
  connect_pin $net external_input
  foreach suffix {"" .gz} {
    set output [make_result_file verilog_input_alias_preserved.v$suffix]
    write_verilog $output
    set previous [input_alias_file_bytes $output]
    if {$previous eq ""} {
      error "valid export was empty"
    }
    connect_pin $net other_input
    set expected "Error: 2749 cannot write Verilog: input ports external_input and other_input share signal net $net"
    if {![catch {write_verilog $output} message] || $message ne $expected} {
      error "expected shared-input rejection, got '$message'"
    }
    if {[input_alias_file_bytes $output] ne $previous} {
      error "rejected export changed an existing output file"
    }
    file delete $output
    if {![catch {write_verilog $output} message] || $message ne $expected} {
      error "expected shared-input rejection for absent output, got '$message'"
    }
    if {[file exists $output]} {
      error "rejected export created an absent output file"
    }
    disconnect_pin $net other_input
  }
}
puts "shared signal inputs: rejected with both ports and net"
puts "rejected writes: plain/gzip outputs preserved and absent paths untouched"

foreach net {power_net ground_net} {
  load_input_alias
  connect_pin $net external_input
  connect_pin $net other_input
  foreach options {{} {-include_pwr_gnd}} {
    write_verilog {*}$options $written_file
    set text [input_alias_file_bytes $written_file]
    foreach port {external_input other_input} {
      if {[string first "assign $net = $port;" $text] >= 0} {
        error "signal input $port drives supply net $net"
      }
    }
  }
}
puts "power/ground nets: no signal-input aliases with either supply setting"
