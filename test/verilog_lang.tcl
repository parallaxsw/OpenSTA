# try to load verilog language file
read_liberty ../examples/sky130hd_tt.lib.gz
read_verilog verilog_lang.v
link_design counter
create_clock -name clk [get_ports clk] -period 50

#confirm that top module was linked

set instance [sta::top_instance]
set cell [$instance cell]
set cell_name [$cell name]
puts "top_instance:\"$cell_name\""
