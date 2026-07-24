# Read design and create clock
read_liberty asap7_small.lib.gz
read_liberty asap7_seq.lib.gz
read_verilog latch_checks.v
link_design top
create_clock -name clk -period 500 {clk}
set_input_delay -clock clk 0 {in}

# Latch checks are enabled by default
report_checks -path_delay max -to [get_pins l1/D] -group_path_count 5

# Turning off latch checks should only report paths from flops
set sta_latch_checks_enabled 0
report_checks -path_delay max -to [get_pins l1/D] -group_path_count 5
report_checks -path_delay max -to [get_pins r2/D] -group_path_count 5

# Enable latch checks again
set sta_latch_checks_enabled 1
report_checks -path_delay max -to [get_pins l1/D] -group_path_count 5
