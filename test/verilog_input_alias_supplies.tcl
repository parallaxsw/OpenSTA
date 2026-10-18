source verilog_input_alias_helpers.tcl

set written_file [make_result_file verilog_input_alias_supplies.v]
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
