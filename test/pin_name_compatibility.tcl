# Opt-in sequential pin aliases: CK/CLK/clk/clock, D/d, Q/q.
proc show { label cmd } {
  set names {}
  foreach_in_collection obj [eval $cmd] {
    lappend names [get_full_name $obj]
  }
  puts "$label = {[lsort $names]}"
}

proc count { cmd } {
  return [sizeof_collection [eval $cmd]]
}

################################################################
# These flops use CK. Querying CLK/clk should miss until the
# compatibility variable is on, then hit the same register clocks.
################################################################
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog ../examples/example1.v
link_design top

puts "######## CK cells, sta_pin_name_compatibility = 0 ########"
puts "default var: $sta_pin_name_compatibility"
show "get_pins r1/CK     " {get_pins -quiet r1/CK}
show "get_pins r1/CLK    " {get_pins -quiet r1/CLK}
show "get_pins */CK      " {get_pins -quiet */CK}
show "get_pins */CLK     " {get_pins -quiet */CLK}
show "get_pins -hier */CLK" {get_pins -quiet -hierarchical */CLK}
show "get_db pins */CLK  " {get_db -quiet pins */CLK}

puts ""
puts "######## CK cells, sta_pin_name_compatibility = 1 ########"
set sta_pin_name_compatibility 1
puts "enabled var: $sta_pin_name_compatibility"
show "get_pins r1/CLK    " {get_pins -quiet r1/CLK}
show "get_pins r1/clk    " {get_pins -quiet r1/clk}
show "get_pins r1/clock  " {get_pins -quiet r1/clock}
show "get_pins */CLK     " {get_pins -quiet */CLK}
show "get_pins */CLK*    " {get_pins -quiet */CLK*}
show "get_pins -hier */CLK" {get_pins -quiet -hierarchical */CLK}
show "get_db pins */CLK  " {get_db -quiet pins */CLK}
show "get_pins r1/d      " {get_pins -quiet r1/d}
show "get_pins r1/q      " {get_pins -quiet r1/q}
# Buffer has no sequential clock; CK/CLK aliases must not invent one.
show "get_pins u1/CLK    " {get_pins -quiet u1/CLK}
show "get_pins u1/A      " {get_pins -quiet u1/A}
# CKE is not a clock-family alias; do not steal enable-style names.
show "get_pins */CKE     " {get_pins -quiet */CKE}

set sta_pin_name_compatibility 0
puts ""
puts "######## CK cells restored 0 ########"
show "get_pins r1/CLK    " {get_pins -quiet r1/CLK}

################################################################
# These flops use CLK. Querying CK/CK* should miss until the
# compatibility variable is on.
################################################################
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

puts ""
puts "######## CLK cells, sta_pin_name_compatibility = 0 ########"
puts "clk pins: [count {get_pins -quiet -hierarchical */CLK}]"
puts "ck pins: [count {get_pins -quiet -hierarchical */CK}]"
puts "ck* pins: [count {get_pins -quiet -hierarchical */CK*}]"
puts "get_db ck: [llength [get_db -quiet pins */CK]]"

puts ""
puts "######## CLK cells, sta_pin_name_compatibility = 1 ########"
set sta_pin_name_compatibility 1
set clk_n [count {get_pins -quiet -hierarchical */CLK}]
set ck_n [count {get_pins -quiet -hierarchical */CK}]
set ckstar_n [count {get_pins -quiet -hierarchical */CK*}]
set db_ck [llength [get_db -quiet pins */CK]]
puts "clk pins: $clk_n"
puts "ck pins: $ck_n"
puts "ck* pins: $ckstar_n"
puts "get_db ck: $db_ck"
puts "ck equals clk: [expr {$ck_n == $clk_n}]"
puts "ck* equals clk: [expr {$ckstar_n == $clk_n}]"
puts "get_db ck equals clk: [expr {$db_ck == $clk_n}]"
puts "leaf CK*: [count {get_pins -quiet *_*/CK*}]"

set sta_pin_name_compatibility 0
puts ""
puts "######## CLK cells restored 0 ########"
puts "ck pins: [count {get_pins -quiet -hierarchical */CK}]"
