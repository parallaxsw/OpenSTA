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

# Test 8: Verify mode persists across operations
puts ""
puts "Test 8: Verify mode persists after design operations"
puts "Current mode before design load:"
report_power_activity_dual_edge

puts ""
puts "Attempting to load existing test design..."
if {[catch {read_liberty test/asap7_simple.lib.gz} err]} {
  puts "Design load skipped (library unavailable for this test environment)"
} else {
  puts "Design loaded successfully"
  puts "Mode after design load:"
  report_power_activity_dual_edge
}

puts ""
puts "=========================================="
puts "All tests completed"
puts "=========================================="
