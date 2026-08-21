# Empty -to used to pass a null PinSeq* into findFaninPins and abort.
# get_pins/get_db of missing objects become an empty Tcl list, and
# tclListSeqPtr returns nullptr when argc == 0.
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd
create_clock -name clk -period 10 [get_ports clk]

proc show { label objects } {
  puts "$label [sizeof_collection $objects]"
}

# Quiet missing pin (same as get_fanin -to of a failed object lookup).
show "missing get_fanin" \
  [get_fanin -to [get_pins -quiet -hierarchical *no_such_pin*]]
show "missing all_fanin" \
  [all_fanin -to [get_pins -quiet -hierarchical *no_such_pin*]]

# Empty Tcl list / empty string (OpenSTA empty-collection encoding).
show "empty list get_fanin" [get_fanin -to {}]
show "empty string get_fanin" [get_fanin -to ""]

# Insts that do not exist, then fanin of that collection (HPM-style).
show "missing insts all_fanin" \
  [all_fanin -to [get_db insts -quiet *no_such_inst*]]

# -only_cells goes through find_fanin_insts, which calls findFaninPins.
show "missing -only_cells" \
  [get_fanin -only_cells -to [get_pins -quiet *no_such_pin*]]
show "missing all_fanout" \
  [all_fanout -from [get_pins -quiet *no_such_pin*]]

# Direct SWIG entry: empty list becomes a null PinSeq*.
show "direct find_fanin_pins" [sta::find_fanin_pins {} 1 0 0 0 0 0]

# A real pin still has fanin.
show "real startpoints" \
  [all_fanin -to [get_ports resp_rdy] -startpoints_only]
