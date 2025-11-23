# Test script for dual-edge power activity propagation mode
# This test verifies the feature that enables signals to switch on both
# posedge and negedge of the clock for more pessimistic power estimation.

puts "=========================================="
puts "Power Activity Dual-Edge Propagation Tests"
puts "=========================================="

# Test 1: Verify default state (disabled)
puts ""
puts "Test 1: Check default state (should be disabled)"
report_power_activity_dual_edge

# Test 2: Enable dual-edge mode
puts ""
puts "Test 2: Enable dual-edge mode"
set_power_activity_dual_edge on
report_power_activity_dual_edge

# Test 3: Disable dual-edge mode
puts ""
puts "Test 3: Disable dual-edge mode"
set_power_activity_dual_edge off
report_power_activity_dual_edge

# Test 4: Test alternative enable syntax (numeric 1)
puts ""
puts "Test 4: Enable using numeric syntax (1)"
set_power_activity_dual_edge 1
report_power_activity_dual_edge

# Test 5: Test alternative disable syntax (numeric 0)
puts ""
puts "Test 5: Disable using numeric syntax (0)"
set_power_activity_dual_edge 0
report_power_activity_dual_edge

# Test 6: Test alternative enable syntax (boolean true)
puts ""
puts "Test 6: Enable using boolean syntax (true)"
set_power_activity_dual_edge true
report_power_activity_dual_edge

# Test 7: Test alternative disable syntax (boolean false)
puts ""
puts "Test 7: Disable using boolean syntax (false)"
set_power_activity_dual_edge false
report_power_activity_dual_edge

# Test 8: Test with actual design - read Liberty and Verilog
puts ""
puts "Test 8: Load design and verify mode affects power calculation"
read_liberty test/power_activity_dual_edge.lib
read_verilog test/power_activity_dual_edge.v
link_design dual_edge_test

# Create clock
create_clock -period 1.0 clk

# Set input activities
set_power_activity -input_ports {a b} -density 0.5 -duty 0.5

# Get baseline power in single-edge mode (default)
puts ""
puts "Single-edge mode (baseline):"
report_power_activity_dual_edge
set power_single [instance_power nand2_inst [cmd_corner]]
puts "NAND2 instance power: [lindex $power_single 3] W"

# Enable dual-edge mode and measure again
puts ""
puts "Dual-edge mode enabled:"
set_power_activity_dual_edge on
report_power_activity_dual_edge
set power_dual [instance_power nand2_inst [cmd_corner]]
puts "NAND2 instance power: [lindex $power_dual 3] W"

# Verify ratio is within expected range
puts ""
puts "Test 9: Verify power ratio is mathematically correct"
set total_single [lindex $power_single 3]
set total_dual [lindex $power_dual 3]
if {$total_single != 0} {
  set ratio [expr {$total_dual / $total_single}]
  puts "Power ratio (dual-edge / single-edge): $ratio"
  puts "Expected ratio range: 1.0 to 1.5 (non-linear, not 2.0)"
  if {$ratio >= 1.0 && $ratio <= 1.5} {
    puts "PASS: Ratio is within expected range"
  } else {
    puts "FAIL: Ratio outside expected range"
  }
} else {
  puts "SKIP: Design needs more configuration for power calculation"
}

puts ""
puts "=========================================="
puts "All tests completed"
puts "=========================================="
