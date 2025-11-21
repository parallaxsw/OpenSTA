# Test script demonstrating dual-edge power activity propagation
# Example: NAND2 gate with input activities
# Input: density(A) = 0.5, density(B) = 0.5, duty(A) = 0.5
# Expected: density(Z) doubles when dual-edge mode is enabled

puts "=========================================="
puts "NAND2 Dual-Edge Power Activity Test"
puts "=========================================="
puts ""

# Create a simple test design with NAND2 gate
# This would require actual Liberty and Verilog files in a real scenario

puts "Note: This test requires:"
puts "  1. A Liberty library file with NAND2 cell"
puts "  2. A Verilog netlist instantiating NAND2"
puts "  3. SDC constraints with clock definition"
puts ""

puts "To run a complete test, execute:"
puts "  ./build/sta examples/nand2_test/run_test.tcl"
puts ""

# Example TCL code structure for the test:
puts "# Example TCL code that would run the test:"
puts ""
puts "# Read design files"
puts "read_liberty nand2_test/simple.lib"
puts "read_verilog nand2_test/nand2_design.v"
puts "link_design NAND2_TOP"
puts ""

puts "# Define clock"
puts "create_clock -period 1.0 clk"
puts ""

puts "# Set input activities"
puts "set_power_activity -pins \{A B\} -density 0.5 -duty 0.5"
puts ""

puts "# Test single-edge mode (default)"
puts "puts {Single-Edge Mode (default):}"
puts "report_power_activity_dual_edge"
puts "set power_se \[instance_power nand2_inst corner0\]"
puts "puts {Output density (Z): [lindex \$power_se 1]}"
puts ""

puts "# Enable dual-edge mode"
puts "set_power_activity_dual_edge on"
puts "puts {Dual-Edge Mode (enabled):}"
puts "report_power_activity_dual_edge"
puts "set power_de \[instance_power nand2_inst corner0\]"
puts "puts {Output density (Z): [lindex \$power_de 1]}"
puts ""

puts "# Verify 2x multiplier"
puts "set ratio \[expr {[lindex \$power_de 1] / [lindex \$power_se 1]}\]"
puts "puts {Ratio (dual-edge / single-edge): \$ratio}"
puts "if {\$ratio > 1.9 && \$ratio < 2.1} {"
puts "  puts {PASS: Dual-edge mode applies 2x multiplier}"
puts "} else {"
puts "  puts {FAIL: Unexpected ratio}"
puts "}"
