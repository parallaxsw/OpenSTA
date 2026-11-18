# Incremental constant propagation from a newly inserted constant-function
# driver (tie cell) to a load connected in a later incremental update.
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog sim_incr_const_func_drvr.v
link_design top
create_clock -name clk -period 10 [get_ports clk]
set_input_delay -clock clk 1.0 [get_ports {en se d}]
set_output_delay -clock clk 1.0 [get_ports {q1 q2}]

# The first timing update makes constant propagation incremental.
report_checks > /dev/null

# Insert a tie-low cell and move icg1/SE onto it.
make_net tie0_net
make_instance tie0 LOGIC0_X1
connect_pin tie0_net tie0/Z
disconnect_pin se icg1/SE
connect_pin tie0_net icg1/SE
check_setup -verbose
puts "tie insert: tie0/Z=[sta::pin_sim_logic_value [get_pins tie0/Z]] icg1/SE=[sta::pin_sim_logic_value [get_pins icg1/SE]]"

# A load connected to the tie net in a later update must see the constant.
disconnect_pin se icg2/SE
connect_pin tie0_net icg2/SE
check_setup -verbose
puts "later load: tie0/Z=[sta::pin_sim_logic_value [get_pins tie0/Z]] icg2/SE=[sta::pin_sim_logic_value [get_pins icg2/SE]]"
