# Sta::makeConcreteParasitics map-overwrite leak repro.
# Two read_spef -min calls share parasitics_name_map_ key "default_min".
# Without the reuse-and-clear guard in Sta::makeConcreteParasitics the
# second call orphans and leaks the first ConcreteParasitics.
# Run under -fsanitize=address to verify no leak after the fix.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
# 1st read_spef
read_spef -min reg1_asap7.spef
# 2nd read_spef
read_spef -min reg1_asap7.spef
