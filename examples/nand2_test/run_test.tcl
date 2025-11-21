# Complete test demonstrating dual-edge power activity propagation
# Example: NAND2 gate with specified input activities

puts "=========================================="
puts "NAND2 Dual-Edge Power Activity Test"
puts "=========================================="
puts ""

# Read design files
puts "Reading design files..."
read_liberty examples/nand2_test/simple.lib
read_verilog examples/nand2_test/nand2_design.v
link_design NAND2_TOP

puts "Design linked successfully"
puts ""

# Define clock
puts "Creating clock constraint..."
create_clock -period 1.0 clk

puts "Clock created: period = 1.0 ns"
puts ""

# Set input activities
puts "Setting input activities..."
puts "  density(A) = 0.5"
puts "  density(B) = 0.5"
puts "  duty(A) = 0.5"
set_power_activity -input_ports {A B} -density 0.5 -duty 0.5

puts ""
puts "=========================================="
puts "SINGLE-EDGE MODE (DEFAULT)"
puts "=========================================="
puts ""

# Report current mode
report_power_activity_dual_edge
puts ""

# Get power in single-edge mode
puts "Instance power for nand2_inst:"
set power_se [instance_power nand2_inst [cmd_corner]]
puts "  Internal:  [lindex $power_se 0] W"
puts "  Switching: [lindex $power_se 1] W"
puts "  Leakage:   [lindex $power_se 2] W"
puts "  Total:     [lindex $power_se 3] W"

set se_switching [lindex $power_se 1]

puts ""
puts "=========================================="
puts "ENABLING DUAL-EDGE MODE"
puts "=========================================="
puts ""

# Enable dual-edge mode
set_power_activity_dual_edge on
report_power_activity_dual_edge
puts ""

# Get power in dual-edge mode
puts "Instance power for nand2_inst (after enabling dual-edge):"
set power_de [instance_power nand2_inst [cmd_corner]]
puts "  Internal:  [lindex $power_de 0] W"
puts "  Switching: [lindex $power_de 1] W"
puts "  Leakage:   [lindex $power_de 2] W"
puts "  Total:     [lindex $power_de 3] W"

set de_switching [lindex $power_de 1]

puts ""
puts "=========================================="
puts "VERIFICATION"
puts "=========================================="
puts ""

# Calculate ratio
if {$se_switching != 0} {
  set ratio [expr {$de_switching / $se_switching}]
  puts "Switching power ratio (dual-edge / single-edge): $ratio"
  
  if {$ratio > 1.9 && $ratio < 2.1} {
    puts "✓ PASS: Dual-edge mode correctly applies 2x multiplier"
  } else {
    puts "✗ FAIL: Unexpected ratio (expected ~2.0)"
  }
} else {
  puts "Note: Switching power is zero (design may need activity input configuration)"
}

puts ""
puts "=========================================="
puts "Test completed"
puts "=========================================="
