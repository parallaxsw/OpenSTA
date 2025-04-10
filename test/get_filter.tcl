# Test get_* -filter

# Read in design and libraries
read_liberty asap7_small.lib.gz
read_verilog reg1_asap7.v
link_design top
create_clock -name clk -period 500 {clk1 clk2 clk3}
create_clock -name vclk -period 1000
create_clock -name vvclk -period 1000

proc run_filter {method filter {target "*"}} {
    puts "\[$method -filter \"$filter\" $target]"
    report_object_full_names [$method -filter "$filter" $target]
}

proc run_bad_filter {method filter {target "*"}} {
    if {[catch {run_filter $method $filter $target} error]} {
        puts "$error"
    } else {
        puts "no error raised"
    }
}

# Test filters for each SDC command
run_filter	get_cells	"liberty_cell==BUFx2_ASAP7_75t_R"
run_filter	get_cells	"(name!~*1&&liberty_cell=~*x2_*)"
run_filter	get_clocks	"is_virtual==0"
run_filter	get_clocks	"is_virtual==1"
run_filter	get_clocks	"is_virtual"
run_filter	get_clocks	"is_virtual&&is_generated"
run_filter	get_clocks	"!(is_generated||name==vvclk)&&is_virtual"
run_filter	get_clocks	"is_virtual&&is_generated==0"
run_filter	get_clocks	"is_virtual&&!is_generated"
run_filter	get_clocks	"is_virtual||is_generated"
run_filter	get_clocks	"is_virtual==0||is_generated"
run_filter	get_lib_cells	"is_buffer==1"
run_filter	get_lib_cells	"is_inverter==0"
run_filter	get_lib_cells	"name=~*x2_*&&!is_buffer"
run_filter	get_lib_pins	"direction==input" BUFx2_ASAP7_75t_R/*
run_filter	get_lib_pins	"direction==output" BUFx2_ASAP7_75t_R/*
run_filter	get_libs	"name==asap7_small"
run_filter	get_nets	"name=~*q"
run_filter	get_pins	"full_name=~r*/*&&(!is_register_clock||direction==output)"
run_filter	get_pins	"direction==input"
run_filter	get_pins	"direction==input&&!name==CLK"
run_filter	get_pins	"direction==output"
run_filter	get_ports	"direction==input"
run_filter	get_ports	"direction==output"

# Test some bad filters
run_bad_filter	get_nets	"(name=~*q"
run_bad_filter	get_nets	"name=~*q)))"
run_bad_filter	get_nets	""
run_bad_filter	get_nets	"name=~*q name=~*v"
run_bad_filter	get_nets	"name=~*q+name=~*v"
run_bad_filter	get_nets	"&&"
run_bad_filter	get_nets	"name=~*q ||"
run_bad_filter	get_nets	"!&&"
