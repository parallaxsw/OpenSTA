# Sta::makeConcreteParasitics map-overwrite leak repro.
# Two read_spef -min calls share parasitics_name_map_ key "default_min".
# Without the reuse-and-clear guard in Sta::makeConcreteParasitics the
# second call orphans and leaks the first ConcreteParasitics.
# Run under -fsanitize=address to verify no leak after the fix.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
set_input_transition 10 {in1 in2 clk1 clk2 clk3}
# 1st read_spef
read_spef -min reg1_asap7.spef
# Mid-flow: query timing, switch delay calculators.
report_checks -path_delay min -digits 3
set_delay_calculator arnoldi
# 2nd read_spef
read_spef -min reg1_asap7.spef
report_checks -path_delay min -digits 3
