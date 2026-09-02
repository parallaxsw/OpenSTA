# Bug 1: Sta::networkChangedNonSdc() (e.g. OpenROAD scan_replace) used to
# call Power::clear(), wiping set_power_activity -global/-input settings.
# Bug 2: instance_powers_ was cleared without resetting instance_powers_valid_,
# so with -global activity set (which skips activity propagation) report_power
# returned all zeros after a network change or a scene switch.
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
set_power_activity -global -activity 2 -duty 1

# Bug 1 and bug 2 (clearNonSdc/powerInvalid): reports must be identical.
report_power
sta::network_changed_non_sdc
report_power

# Bug 2 (ensureActivities scene switch): scene2 must not be zero.
define_scene scene1 -liberty asap7_small
define_scene scene2 -liberty asap7_small
report_power -scene scene1
report_power -scene scene2
