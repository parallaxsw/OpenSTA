read_liberty ../examples/sky130hd_tt.lib
read_verilog ../examples/gcd_sky130hd.v
link_design gcd

set_dont_use sky130_fd_sc_hd__a2111o_1
set_dont_touch sky130_fd_sc_hd__a2111o_1
echo [get_db program_short_name]
echo [get_object_name [get_ports req_rdy]]
echo [get_object_name [query_objects [get_ports req_rdy]]]
all_fanin -to [get_ports resp_rdy]
all_fanout -from [get_ports req_rdy]
