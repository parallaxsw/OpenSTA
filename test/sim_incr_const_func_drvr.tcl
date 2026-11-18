# Incremental constant propagation from a newly inserted constant-function
# driver (tie cell) to loads connected in a subsequent incremental update.
read_liberty ../examples/nangate45_typ.lib.gz
read_verilog sim_incr_const_func_drvr.v
link_design top
create_clock -name clk -period 10 [get_ports clk]
set_input_delay -clock clk 1.0 [get_ports {en d}]
set_output_delay -clock clk 1.0 [get_ports {q1 q2}]

# Initial timing update enables incremental Sim constant propagation.
report_checks > /dev/null

# Insert a tie-low cell and connect icg1/SE to it (like repair_tie_fanout).
make_net tie0_net
make_instance tie0 LOGIC0_X1
connect_pin tie0_net tie0/Z
disconnect_pin se_init icg1/SE
connect_pin tie0_net icg1/SE

# First incremental update propagates tie0/Z -> icg1/SE.
check_setup -verbose
puts "after tie insert: tie0/Z=[sta::pin_sim_logic_value [get_pins tie0/Z]] icg1/SE=[sta::pin_sim_logic_value [get_pins icg1/SE]]"

# Connect a second clock gate's SE pin to the existing tie-low net (like CTS ICG cloning).
make_net gclk2
make_instance icg2 CLKGATETST_X1
connect_pin clk icg2/CK
connect_pin en icg2/E
connect_pin tie0_net icg2/SE
connect_pin gclk2 icg2/GCK
make_instance r2 DFF_X1
connect_pin gclk2 r2/CK
connect_pin d r2/D
connect_pin q2 r2/Q

# Second incremental update must inherit constant 0 from tie0/Z on icg2/SE.
check_setup -verbose
puts "after clone connect: tie0/Z=[sta::pin_sim_logic_value [get_pins tie0/Z]] icg2/SE=[sta::pin_sim_logic_value [get_pins icg2/SE]]"
