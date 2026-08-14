# Commands

This page is generated from the live command registry.
Do not edit it by hand; rebuild `sta` to regenerate.

Use `help <command>` in the Tcl interpreter for the same text.

## all_clocks

```
all_clocks
```

The `all_clocks` command returns a list of all clocks that have been defined.

## all_inputs

```
all_inputs [-no_clocks]
```

The `all_inputs` command returns a list of all input and bidirect ports of the current design.

### Options

`-no_clocks`
: Exclude inputs defined as clock sources.

## all_outputs

```
all_outputs
```

The `all_outputs` command returns a list of all output and bidirect ports of the design.

## all_registers

```
all_registers [-clock clocks] [-rise_clock clocks] [-fall_clock clocks] [-cells] [-data_pins] [-clock_pins] [-async_pins] [-output_pins] [-level_sensitive] [-edge_triggered]
```

The `all_registers` command returns a list of  register instances or register pins in the design. Options allow the list of registers to be restricted in various ways. The `-clock` keyword restrcts the registers to those that are clocked by a set of clocks. The `-cells` option returns the list of registers or latches (the default). The `-data_pins`, `-clock_pins`, `-async_pins` and `-output_pins` options cause `all_registers` to return a list of register pins rather than instances.

### Options

`-clock`
: `clock_names`: A list of clock names. Only registers clocked by these clocks are returned.

`-rise_clock`
: Only registers clocked by the rising edge of these clocks are returned.

`-fall_clock`
: Only registers clocked by the falling edge of these clocks are returned.

`-cells`
: Return a list of register instances.

`-data_pins`
: Return the register data pins.

`-clock_pins`
: Return the register clock pins.

`-async_pins`
: Return the register set/clear pins.

`-output_pins`
: Return the register output pins.

`-level_sensitive`
: Return level-sensitive latches.

`-edge_triggered`
: Return edge-triggered registers.

## check_setup

```
check_setup [-verbose] [-no_input_delay] [-no_output_delay] [-multiple_clock] [-no_clock] [-unconstrained_endpoints] [-loops] [-generated_clocks] [> filename] [>> filename]
```

The `check_setup` command performs sanity checks on the design. Individual checks can be performed with the keywords. If no check keywords are specified all checks are performed. Checks that fail are reported as warnings. If no checks fail nothing is reported. The command returns 1 if there are no warnings for use in scripts.

### Options

`-verbose`
: Show offending objects rather than just error counts.

`-no_input_delay`
: Check for inputs that do not have a `set_input_delay` command.

`-no_output_delay`
: Check for outputs that do not have a `set_output_delay` command.

`-multiple_clock`
: Check register/latch clock pins for multiple clocks.

`-no_clock`
: Check register/latch clock pins for a clock.

`-unconstrained_endpoints`
: Check path endpoints for timing constraints (timing check or `set_output_delay`).

`-loops`
: Check for combinational logic loops.

`-generated_clocks`
: Check that generated clock source pins have been defined as clocks.

## connect_pin

```
connect_pin net pin
```

The `connect_pin` command connects a port or instance pin to a net.

## create_clock

```
create_clock [-name name] [-period period] [-waveform waveform] [-add] [-comment comment] [pins]
```

The `create_clock` command defines the waveform of a clock used by the design.

If no pin_list is specified the clock is virtual. A virtual clock can be refered to by name in input arrival and departure time commands but is not attached to any pins in the design.

If no clock name is specified the name of the first pin is used as the clock name.

If a wavform is not specified the clock rises at zero and falls at half the clock period. The waveform is a list with time the clock rises as the first element and the time it falls as the second element.

If a clock is already defined on a pin the clock is redefined using the new clock parameters. If multiple clocks drive the same pin, use the `-add` option to prevent the existing definition from being overwritten.

The following command creates a clock with a period of 10 time units that rises at time 0 and falls at 5 time units on the pin named clk1.

```
create_clock -period 10 clk1
```

The following command creates a clock with a period of 10 time units that is high at time zero, falls at time 2 and rises at time 8. The clock drives three pins named clk1, clk2, and clk3.

```
create_clock -period 10 -waveform {8 2} -name clk {clk1 clk2 clk3}
```

### Options

`-name`
: `clock_name`: The name of the clock.

`-period`
: `period`: The clock period.

`-waveform`
: `edge_list`: A list of edge rise and fall time.

`-add`
: Add this clock to the clocks on pin_list.

`-comment`
: Comment string saved with the constraint.

## create_generated_clock

```
create_generated_clock [-name clock_name] -source master_pin [-master_clock clock] [-divide_by divisor | -multiply_by multiplier] [-duty_cycle duty_cycle] [-invert] [-edges edge_list] [-edge_shift edge_shift_list] [-combinational] [-add] [-comment comment] port_pin_list
```

The `create_generated_clock` command is used to generate a clock from an existing clock definition. It is used to model clock generation circuits such as clock dividers and phase locked loops.

The `-divide_by`, `-multiply_by` and `-edges` arguments are mutually exclusive.

The `-multiply_by` option is used to generate a higher frequency clock from the source clock. The period of the generated clock is divided by multiplier. The clock multiplier must be a positive integer. If a duty cycle is specified the generated clock rises at zero and falls at period * duty_cycle / 100. If no duty cycle is specified the source clock edge times are divided by multiplier.

The `-divide_by` option is used to generate a lower frequency clock from the source clock. The clock divisor must be a positive integer. If the clock divisor is a power of two the source clock period is multiplied by divisor, the clock rise time is the same as the source clock, and the clock fall edge is one half period later. If the clock divisor is not a power of two the source clock waveform edge times are multiplied by divisor.

The `-edges` option forms the generated clock waveform by selecting edges from the source clock waveform.

If the `-invert` option is specified the waveform derived above is inverted.

If a clock is already defined on a pin the clock is redefined using the new clock parameters. If multiple clocks drive the same pin, use the `-add` option to prevent the existing definition from being overwritten.

In the example show below generates a clock named gclk1 on register output pin r1/Q by dividing it by four.

```
create_clock -period 10 -waveform {1 8} clk1
create_generated_clock -name gclk1 -source clk1 -divide_by 4 r1/Q
```

The generated clock has a period of 40, rises at time 1 and falls at time 21.

In the example shown below the duty cycle is used to define the derived clock waveform.

```
create_generated_clock -name gclk1 -source clk1 -duty_cycle 50  -multiply_by 2 r1/Q
```

The generated clock has a period of 5, rises at time .5 and falls at time 3.

In the example shown below the first, third and fifth source clock edges are used to define the derived clock waveform.

```
create_generated_clock -name gclk1 -source clk1 -edges {1 3 5} r1/Q
```

The generated clock has a period of 20, rises at time 1 and falls at time 11.

### Options

`-name`
: `clock_name`: The name of the generated clock.

`-source`
: `master_pin`: A pin or port in the fanout of the master clock that is the source of the generated clock.

`-master_clock`
: `master_clock`: Use `-master_clock` to specify which source clock to use when multiple clocks are present on master_pin.

`-divide_by`
: `divisor`: Divide the master clock period by divisor.

`-multiply_by`
: `multiplier`: Multiply the master clock period by multiplier.

`-duty_cycle`
: `duty_cycle`: The percent of the period that the generated clock is high (between 0 and 100).

`-invert`
: Invert the master clock.

`-edges`
: `edge_list`: List of master clock edges to use in the generated clock. Edges are numbered from 1. edge_list must be 3 edges long.

`-edge_shift`
: `shift_list`: Not supported.

`-combinational`
: The generated clock is combinational, equivalent to `-divide_by 1`.

`-add`
: Add this clock to the existing clocks on pin_list.

`-comment`
: Comment string saved with the constraint.

## create_voltage_area

```
create_voltage_area [-name name] [-coordinate coordinates] [-guard_band_x guard_x] [-guard_band_y guard_y] cells
```

This command is parsed and ignored by timing analysis.

### Options

`-name`
: Voltage area name. Ignored.

`-coordinate`
: Voltage area coordinates. Ignored.

`-guard_band_x`
: X guard band. Ignored.

`-guard_band_y`
: Y guard band. Ignored.

## current_design

```
current_design [design]
```

Set or report the current design. OpenSTA only supports one design.

## current_instance

```
current_instance [instance]
```

Set or report the current instance used for relative name lookup.

## define_corners

```
define_corners corner1 [corner2]...
```

The `define_corners` command is deprecated. Use `define_scene` instead. It is supported for compatibility with older scripts that define analysis corners before `read_liberty`, but should not be used with MCMM flows.

## define_property

```
define_property -object_type scene|mode|library|liberty_library|cell|liberty_cell|port|liberty_port|instance|pin|net|clock -type bool|float|string property
```

The `define_property` command defines a user property that can be set with `set_property` and read with `get_property`. User properties can also be used in `-filter` expressions.

### Options

`-object_type`
: Object type the property applies to.

`-type`
: - `bool`: Boolean value.
  - `float`: Floating point value.
  - `string`: String value.

## define_scene

```
define_scene name -mode mode_name -liberty liberty_files  | -liberty_min liberty_min_files -liberty_max liberty_max_files [-spef spef_file | -spef_min spef_min_file -spef_max spef_max_file]
```

The `define_scene` command defines a scene for a mode (SDC), liberty files and spef parasitics. Define scenes after reading Liberty libraries and SPEF parasitics.

Liberty files are specified with the name of the Liberty library or the filename of the Liberty file. If a filename is used, it must be the same as the filename used to read the library with `read_liberty`.

Use `get_scenes` to find defined scenes.

### Options

`-mode`
: The SDC mode to use.

`-liberty`
: Liberty library name or filename used with `read_liberty`.

`-liberty_min`
: Min-delay Liberty library name or filename.

`-liberty_max`
: Max-delay Liberty library name or filename.

`-spef`
: SPEF parasitics name from `read_spef -name`.

`-spef_min`
: Min-delay SPEF parasitics name.

`-spef_max`
: Max-delay SPEF parasitics name.

## delete_clock

```
delete_clock [-all] clocks
```

Delete clocks.

### Options

`-all`
: Delete all clocks.

## delete_from_list

```
delete_from_list list delete
```

Remove objects from a list.

## delete_generated_clock

```
delete_generated_clock [-all] clocks
```

Delete generated clocks.

### Options

`-all`
: Delete all generated clocks.

## delete_instance

```
delete_instance inst
```

The network editing command `delete_instance` removes an instance from the design.

## delete_net

```
delete_net net
```

The network editing command `delete_net` removes a net from the design.

## disconnect_pin

```
disconnect_pin net -all|pin
```

Disconnects a port or pin from a net. Parasitics connected to the pin are deleted.

### Options

`-all`
: Disconnect all pins from the net.

## elapsed_run_time

```
elapsed_run_time
```

Returns the total clock run time in seconds as a float.

## find_timing_paths

```
find_timing_paths [-from from_list|-rise_from from_list|-fall_from from_list] [-through through_list|-rise_through through_list|-fall_through through_list] [-to to_list|-rise_to to_list|-fall_to to_list] [-path_delay min|min_rise|min_fall|max|max_rise|max_fall|min_max] [-unconstrained]
     [-scenes scenes] [-group_path_count path_count]  [-endpoint_path_count path_count] [-unique_paths_to_endpoint] [-unique_edges_to_endpoint] [-slack_max slack_max] [-slack_min slack_min] [-sort_by_slack] [-path_group group_name]
```

The `find_timing_paths` command returns a list of path objects for scripting. Use the `get_property` function to access properties of the paths.

### Options

`-from`
: Return paths from a list of clocks, instances, ports, register clock pins, or latch data pins.

`-rise_from`
: Return paths from the rising edge of clocks, instances, ports, register clock pins, or latch data pins.

`-fall_from`
: Return paths from the falling edge of clocks, instances, ports, register clock pins, or latch data pins.

`-through`
: Return paths through a list of instances, pins or nets.

`-rise_through`
: Return rising paths through a list of instances, pins or nets.

`-fall_through`
: Return falling paths through a list of instances, pins or nets.

`-to`
: Return paths to a list of clocks, instances, ports or pins.

`-rise_to`
: Return rising paths to a list of clocks, instances, ports or pins.

`-fall_to`
: Return falling paths to a list of clocks, instances, ports or pins.

`-path_delay`
: - `min`: Return min path (hold) checks.
  - `min_rise`: Return min path (hold) checks for rising endpoints.
  - `min_fall`: Return min path (hold) checks for falling endpoints.
  - `max`: Return max path (setup) checks.
  - `max_rise`: Return max path (setup) checks for rising endpoints.
  - `max_fall`: Return max path (setup) checks for falling endpoints.
  - `min_max`: Return min and max path (setup and hold) checks.

`-unconstrained`
: Report unconstrained paths also.

`-scenes`
: `scenes`: Return paths for these scenes. The default is all scenes.

`-group_path_count`
: `path_count`: The number of paths to return in each path group.

`-endpoint_path_count`
: `endpoint_path_count`: The number of paths to return for each endpoint.

`-unique_paths_to_endpoint`
: Return multiple paths to an endpoint that traverse different pins without showing multiple paths with different rise/fall transitions.

`-unique_edges_to_endpoint`
: When multiple paths to an endpoint are requested, only the worst path through the same pins and rise/fall edges is returned.

`-slack_max`
: `max_slack`: Return paths with slack less than max_slack.

`-slack_min`
: `min_slack`: Return paths with slack greater than min_slack.

`-sort_by_slack`
: Sort paths by slack rather than slack within path groups.

`-path_group`
: `groups`: Return paths in path groups. Paths in all groups are returned if this option is not specified.

## get_cells

```
get_cells [-hierarchical] [-hsc separator] [-filter expr] [-regexp] [-nocase] [-quiet] [-of_objects objects] [patterns]
```

The `get_cells` command returns a list of all cell instances that match patterns.

### Options

`-hierarchical`
: Searches hierarchy levels below the current instance for matches.

`-hsc`
: `separator`: Character to use to separate hierarchical instance names in patterns.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-of_objects`
: The name of a pin or net, a list of pins returned by `get_pins`, or a list of nets returned by `get_nets`. The `-hierarchical` option cannot be used with `-of_objects`.

## get_clocks

```
get_clocks [-regexp] [-nocase] [-quiet] [-filter expr] [patterns]
```

The `get_clocks` command returns a list of all clocks that have been defined.

### Options

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

## get_fanin

```
get_fanin -to sink_list [-flat] [-only_cells] [-startpoints_only] [-levels level_count] [-pin_levels pin_count] [-trace_arcs timing|enabled|all]
```

The `get_fanin`  command returns traverses the design from sink_list pins, ports or nets backwards and return the fanin pins or instances.

### Options

`-to`
: `sink_list`: List of pins, ports, or nets to find the fanin of. For nets, the fanin of driver pins on the nets are returned.

`-flat`
: With `-flat` pins in the fanin at any hierarchy level are returned. Without `-flat` only pins at the same hierarchy level as the sinks are returned.

`-only_cells`
: Return the instances connected to the pins in the fanin.

`-startpoints_only`
: Only return pins that are startpoints.

`-levels`
: `level_count`: Only return pins within level_count instance traversals.

`-pin_levels`
: `pin_count`: Only return pins within pin_count pin traversals.

`-trace_arcs`
: - `timing`: Only trace through timing arcs that are not disabled.
  - `enabled`: Only trace through timing arcs that are not disabled.
  - `all`: Trace through all arcs, including disabled ones.

## get_fanout

```
get_fanout -from source_list [-flat] [-only_cells] [-endpoints_only] [-levels level_count] [-pin_levels pin_count] [-trace_arcs timing|enabled|all]
```

The `get_fanout`  command returns traverses the design from source_list pins, ports or nets backwards and return the fanout pins or instances.

### Options

`-from`
: `source_list`: List of pins, ports, or nets to find the fanout of. For nets, the fanout of load pins on the nets are returned.

`-flat`
: With `-flat` pins in the fanin at any hierarchy level are returned. Without `-flat` only pins at the same hierarchy level as the sinks are returned.

`-only_cells`
: Return the instances connected to the pins in the fanout.

`-endpoints_only`
: Only return pins that are endpoints.

`-levels`
: `level_count`: Only return pins within level_count instance traversals.

`-pin_levels`
: `pin_count`: Only return pins within pin_count pin traversals.

`-trace_arcs`
: - `timing`: Only trace through timing arcs that are not disabled.
  - `enabled`: Only trace through timing arcs that are not disabled.
  - `all`: Trace through all arcs, including disabled ones.

## get_full_name

```
get_full_name object
```

Return the name of object. Equivalent to [`get_property` object full_name].

## get_lib_cells

```
get_lib_cells [-hsc separator] [-regexp] [-nocase] [-quiet] [-filter expr] [-of_objects objects] [patterns]
```

The `get_lib_cells` command returns a list of library cells that match pattern. The library name can be prepended to the cell name pattern with the separator character, which defaults to `hierarchy_separator`.

### Options

`-hsc`
: `separator`: Character that separates the library name and cell name in patterns. Defaults to '/'.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-of_objects`
: A list of instance objects.

## get_lib_pins

```
get_lib_pins [-hsc separator] [-regexp] [-nocase] [-quiet] [-filter expr] [-of_objects objects] [patterns]
```

The `get_lib_pins` command returns a list of library ports that match pattern.     Use separator to separate the library and cell name patterns from the port name in pattern.

### Options

`-hsc`
: `separator`: Character that separates the library name, cell name and port name in pattern. Defaults to '/'.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-of_objects`
: A list of library cell objects.

## get_libs

```
get_libs [-regexp] [-nocase] [-quiet] [-filter expr] [patterns]
```

The `get_libs` command returns a list of clocks that match patterns.

### Options

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

## get_modes

```
get_modes [-filter expr] [mode_name]
```

The `get_modes` command finds SDC modes matching a pattern.

### Options

`-filter`
: A filter expression. See the section "Filter Expressions".

## get_name

```
get_name object
```

Return the name of object. Equivalent to [`get_property` object name].

## get_nets

```
get_nets [-hierarchical] [-hsc separator] [-regexp] [-nocase] [-quiet] [-filter expr] [-of_objects objects] [patterns]
```

The `get_nets` command returns a list of all nets that match patterns.

### Options

`-hierarchical`
: Searches hierarchy levels below the current instance for matches.

`-hsc`
: `separator`: Character that separates the library name, cell name and port name in pattern. Defaults to '/'.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-of_objects`
: The name of a pin or instance, a list of pins returned by `get_pins`, or a list of instances returned by `get_cells`. The `-hierarchical` option cannot be used with `-of_objects`.

## get_pins

```
get_pins [-hierarchical] [-hsc separator] [-quiet] [-filter expr] [-regexp] [-nocase] [-of_objects objects] [patterns]
```

The `get_pins` command returns a list of all instance pins that match patterns.

A useful idiom to find the driver pin for a net is the following.

```
get_pins -of_objects [get_net net_name] -filter "direction==output"
```

### Options

`-hierarchical`
: Searches hierarchy levels below the current instance for matches.

`-hsc`
: `separator`: Character that separates the library name, cell name and port name in pattern. Defaults to '/'.

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-of_objects`
: The name of a net or instance, a list of nets returned by `get_nets`, or a list of instances returned by `get_cells`. The `-hierarchical` option cannot be used with `-of_objects`.

## get_ports

```
get_ports [-quiet] [-filter expr] [-regexp] [-nocase] [-of_objects objects] [patterns]
```

The `get_ports` command returns a list of all top level ports that match patterns.

### Options

`-quiet`
: Do not report an error if no objects match.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

`-regexp`
: Match patterns as regular expressions.

`-nocase`
: Case-insensitive matching. Only valid with `-regexp`.

`-of_objects`
: The name of  net or a list of nets returned by `get_nets`.

## get_property

```
get_property [-object_type library|liberty_library|cell|liberty_cell|instance|pin|net|port|clock|timing_arc] object property
```

The `get_property` command returns a property of an object. Properties for each object type are shown below.

| Object type | Properties |
| --- | --- |
| cell (SDC lib_cell) | `base_name`, `filename`, `full_name`, `library`, `name` |
| clock | `full_name`, `is_generated`, `is_propagated`, `is_virtual`, `name`, `period`, `sources` |
| edge | `delay_max_fall`, `delay_min_fall`, `delay_max_rise`, `delay_min_rise`, `full_name`, `from_pin`, `sense`, `to_pin` |
| instance (SDC cell) | `cell`, `full_name`, `is_buffer`, `is_clock_gate`, `is_hierarchical`, `is_inverter`, `is_macro`, `is_memory`, `liberty_cell`, `name`, `ref_name` |
| liberty_cell (SDC lib_cell) | `area`, `base_name`, `dont_use`, `filename`, `full_name`, `is_buffer`, `is_inverter`, `is_memory`, `library`, `name` |
| liberty_port (SDC lib_pin) | `capacitance`, `direction`, `drive_resistance`, `drive_resistance_max_fall`, `drive_resistance_max_rise`, `drive_resistance_min_fall`, `drive_resistance_min_rise`, `full_name`, `intrinsic_delay`, `intrinsic_delay_max_fall`, `intrinsic_delay_max_rise`, `intrinsic_delay_min_fall`, `intrinsic_delay_min_rise`, `is_register_clock`, `lib_cell`, `name` |
| library | `filename` (Liberty library only), `name`, `full_name` |
| mode | `name`, `full_name` |
| net | `full_name`, `name` |
| path (PathEnd) | `endpoint`, `endpoint_clock`, `endpoint_clock_pin`, `slack`, `startpoint`, `startpoint_clock`, `points` |
| pin | `activity`, `slew_max_fall`, `slew_max_rise`, `slew_min_fall`, `slew_min_rise`, `clocks`, `clock_domains`, `direction`, `full_name`, `is_hierarchical`, `is_port`, `is_register_clock`, `lib_pin_name`, `name`, `slack_max`, `slack_max_fall`, `slack_max_rise`, `slack_min`, `slack_min_fall`, `slack_min_rise` |
| point (PathRef) | `arrival`, `pin`, `required`, `slack` |
| port | `activity`, `slew_max_fall`, `slew_max_rise`, `slew_min_fall`, `slew_min_rise`, `direction`, `full_name`, `liberty_port`, `name`, `slack_max`, `slack_max_fall`, `slack_max_rise`, `slack_min`, `slack_min_fall`, `slack_min_rise` |
| scene | `name`, `full_name` |

The pin `activity` property is a list of activity (transitions per second), duty cycle, and origin. Origin is one of `global` (`set_power_activity -global`), `input` (`set_power_activity -input`), `user` (`set_power_activity -input_ports`/`-pins`), `vcd` (`read_vcd`), `saif` (`read_saif`), `propagated`, `clock` (`create_clock`/`create_generated_clock`), or `constant` (Verilog tie high/low, `set_case_analysis`, `set_logic_one`/`zero`/`dc`).

### Options

`-object_type`
: `object_type`: The type of object when it is specified as a name.
  cell|pin|net|port|clock|library|library_cell|library_pin|timing_arc

## get_scenes

```
get_scenes [-modes mode_names] [-filter expr] scene_names
```

The `get_scenes` command is used to find the scenes matching a pattern or that use an SDC mode.

### Options

`-modes`
: Return scenes that use these SDC modes.

`-filter`
: A filter expression. See the section "Filter Expressions".

## get_timing_edges

```
get_timing_edges [-from from_pin] [-to to_pin] [-of_objects objects] [-filter expr]
```

The `get_timing_edges` command returns a list of timing edges (arcs) to, from or between pins. The result can be passed to `get_property` or `set_disable_timing`.

### Options

`-from`
: `from_pin`: A list of pins.

`-to`
: `to_pin`: A list of pins.

`-of_objects`
: A list of instances or library cells. The `-from` and `-to` options cannot be used with `-of_objects`.

`-filter`
: A filter expression of the form
    "property==value"
  where property is a property supported by the `get_property` command.  See the section "Filter Expressions" for additional forms.

## group_path

```
group_path -name group_name [-weight weight] [-critical_range range] [-default] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list]
```

The `group_path` command is used to group paths reported by the `report_checks` command. See `set_false_path` for a description of allowed from_list, through_list and to_list objects.

### Options

`-name`
: `group_name`: The name of the path group.

`-weight`
: `weight`: Not supported.

`-critical_range`
: `range`: Not supported.

`-default`
: Restore the paths in the path group `-from`/`-to`/`-through`/`-to` to their default path group.

`-comment`
: Comment string saved with the constraint.

`-from`
: Group paths from a list of clocks, instances, ports, register clock pins, or latch data pins.

`-rise_from`
: Group  paths from the rising edge of clocks, instances, ports, register clock pins, or latch data pins.

`-fall_from`
: Group paths from the falling edge of clocks, instances, ports, register clock pins, or latch data pins.

`-through`
: Group paths through a list of instances, pins or nets.

`-rise_through`
: Group rising paths through a list of instances, pins or nets.

`-fall_through`
: Group falling paths through a list of instances, pins or nets.

`-to`
: Group paths to a list of clocks, instances, ports or pins.

`-rise_to`
: Group rising paths to a list of clocks, instances, ports or pins.

`-fall_to`
: Group falling paths to a list of clocks, instances, port-s or pins.

## help

```
help [-verbose] [pattern]
```

Print command usage. With a single match, print the description and options. Use `-verbose` to print full `help` for every match.

### Options

`-verbose`
: Print full descriptions even when multiple commands match.

## include

```
include [-e|-echo] [-v|-verbose] filename [> filename] [>> filename]
```

Read STA/SDC/Tcl commands from filename.

The `include` command stops and reports any errors encountered while reading a file unless `sta_continue_on_error` is 1.

### Options

`-echo`
: Print each command before evaluating it.

`-verbose`
: Print each command before evaluating it as well as the result it returns.

## link_design

```
link_design [-no_black_boxes] [top_cell_name]
```

Link (elaborate, flatten) the top-level cell `cell_name`. The design must be linked after reading netlist and library files. The default value of `cell_name` is the current design.

By default the linker creates empty black-box cells for instances that reference undefined cells. Use `-no_black_boxes` to report an error and fail the link instead.

The `link_design` command returns 1 if the link succeeds and 0 if it fails.

### Options

`-no_black_boxes`
: Do not make empty "black box" cells for instances that reference undefined cells.

## log_begin

```
log_begin filename
```

The `log_begin` command copies all subsequent command output to a file until `log_end` is called.

## log_end

```
log_end
```

The `log_end` command stops copying command output to a file started with `log_begin`.

## make_instance

```
make_instance inst_path lib_cell
```

The `make_instance` command makes an instance of library cell lib_cell.

## make_net

```
make_net net_path
```

Creates a net for each hierarchical net name.

## make_port

```
make_port port_name direction
```

The `make_port` command creates a port on the top-level cell. `direction` is `input`, `output`, `bidirect`, `tristate`, `internal`, `power`, or `ground`.

## read_liberty

```
read_liberty [-corner corner] [-min] [-max] [-infer_latches] filename
```

The `read_liberty` command reads a Liberty format library file. The first library that is read sets the units used by SDC/Tcl commands and reporting. The include_file attribute is supported.

Some Liberty libraries do not include latch groups for cells that describe transparent latches. In that situation the `-infer_latches` command flag can be used to infer the latches. The timing arcs required for a latch to be inferred should look like the following:

```
cell (inferred_latch) {
  pin(D) {
    direction : input ;
    timing () {
      related_pin : "E" ;
      timing_type : setup_falling ;
    }
    timing () {
      related_pin : "E" ;
      timing_type : hold_falling ;
    }
  }
  pin(E) {
    direction : input;
  }
  pin(Q) {
    direction : output ;
    timing () {
      related_pin : "D" ;
    }
    timing () {
      related_pin : "E" ;
      timing_type : rising_edge ;
    }
  }
}
```

In this example a positive level-sensitive latch is inferred.

Files compressed with gzip are automatically uncompressed.

### Options

`-corner`
: Deprecated. Use `define_scene` to assign Liberty libraries to a scene.

`-min`
: Use the library for min-delay (hold) analysis.

`-max`
: Use the library for max-delay (setup) analysis.

`-infer_latches`
: Infer latches from timing arcs when the Liberty file has no latch groups.

## read_power_activities

```
read_power_activities [-scope scope] -vcd filename
```

The `read_power_activities` command is deprecated. Use `read_vcd` instead.

### Options

`-scope`
: The VCD scope of the current design. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.

`-vcd`
: VCD file to read. Use `read_vcd` instead.

## read_saif

```
read_saif [-scope scope] filename
```

The `read_saif` command reads a SAIF (Switching Activity Interchange Format) file from a Verilog simulation and extracts pin activities and duty cycles for use in power estimation. Files compressed with gzip are supported. Annotated activities are propagated to the fanout of the annotated pins.

### Options

`-scope`
: The SAIF scope of the current design to extract simulation data. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.

## read_sdc

```
read_sdc [-echo] [-mode mode_name] filename
```

Read SDC commands from filename.

If the mode does not exist it is created. Multiple SDC files can append commands to a mode by using the `-mode_name` argument for each one. If no `-mode` arguement is is used the commands are added to the current  mode.

The `read_sdc` command stops and reports any errors encountered while reading a file unless `sta_continue_on_error` is 1.

Files compressed with gzip are automatically uncompressed.

### Options

`-echo`
: Print each command before evaluating it.

`-mode`
: Mode for the SDC commands in the file.

## read_sdf

```
read_sdf [-path path] [-scene scene] [-cond_use min|max|min_max] [-unescaped_dividers] filename
```

Read SDF delays from a file. The min and max values in the SDF tuples are used to annotate delays. Typical values in the SDF tuples are ignored. If multiple scenes are defined `-scene` must be specified. SDC annotation for MCMM analysis must follow the scene definitions.

Files compressed with gzip are automatically uncompressed.

INCREMENT is supported as an alias for INCREMENTAL.

The following SDF statements are not supported.

```
PORT
INSTANCE wildcards
```

### Options

`-path`
: Hierarchical instance path prefix for SDF annotation.

`-scene`
: Scene delays to annotate.

`-cond_use`
: - `min`: Use SDF COND delays for min analysis.
  - `max`: Use COND delays for max analysis.
  - `min_max`: Use COND delays for min and max analysis.

`-unescaped_dividers`
: With this option path names in the SDF do not have to escape hierarchy dividers when the path name is escaped. For example, the escaped Verilog name "\inst1/inst2 " can be referenced as "inst1/inst2". The correct SDF name is "inst1\/inst2", since the divider does not represent a change in hierarchy in this case.

## read_spef

```
read_spef [-name spef_name]
   [-corner corner] [-min] [-max] [-path path] [-pin_cap_included] [-keep_capacitive_coupling] [-coupling_reduction_factor factor] [-reduce] filename
```

The `read_spef` command reads a file of net parasitics in SPEF format. Use the `-report_parasitic_annotation` command to check for nets that are not annotated.

Files compressed with gzip are automatically uncompressed.

Separate min/max parasitics can be annotated for each scene.

```
read_spef -name min spef1
read_spef -name max spef2
define_scene -mode mode1 -spef_min min -spef_max max
```

Coupling capacitors are multiplied by the `-coupling_reduction_factor` when a parasitic network is reduced.

The following SPEF constructs are ignored.

```
*DESIGN_FLOW (all values are ignored)
*S slews
*D driving cell
*I pin capacitances (library cell capacitances are used instead)
*Q r_net load poles
*K r_net load residues
```

If the SPEF file contains triplet values the first value is used.

Parasitic networks (DSPEF) can be annotated on hierarchical blocks using the `-path` argument to specify the instance path to the block. Parasitic networks in the higher level netlist are stitched together at the hierarchical pins of the blocks.

### Options

`-name`
: The name of the SPEF parasitics to use for defining scenes. The default is the base name of filename.

`-corner`
: Process corner to annotate. Deprecated; use `-name` and `define_scene`.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-path`
: Hierarchical block instance path to annotate with  parasitics.

`-pin_cap_included`
: SPEF pin capacitances are included (library pin capacitances are not added).

`-keep_capacitive_coupling`
: Keep coupling capacitors in parasitic networks rather than converting them to grounded capacitors.

`-coupling_reduction_factor`
: `factor`: Factor to multiply coupling capacitance by when reducing parasitic networks. The default value is 1.0.

`-reduce`
: Reduce parasitic networks to the form used by the current delay calculator.

## read_vcd

```
read_vcd [-scope scope] [-mode mode_name] [-begin_time begin_time] [-end_time end_time] filename
```

The `read_vcd` command reads a VCD (Value Change Dump) file from a Verilog simulation and extracts pin activities and duty cycles for use in power estimation. Files compressed with gzip are supported. Annotated activities are propagated to the fanout of the annotated pins.

### Options

`-scope`
: The VCD scope of the current design to extract simulation data. Typically the test bench name and design under test instance name. Scope levels are separated with '/'.

`-mode`
: Mode to annotate activities.

`-begin_time`
: Ignore VCD activity before this time.

`-end_time`
: Ignore VCD activity after this time.

## read_verilog

```
read_verilog filename
```

The `read_verilog` command reads a gate level verilog netlist. After all verilog netlist and Liberty libraries are read the design must be linked with the `link_design` command.

Verilog 2001 module port declaratations are supported. An example is shown below.

```
module top (input in1, in2, clk1, clk2, clk3,
            output out);
```

Files compressed with gzip are automatically uncompressed.

## replace_cell

```
replace_cell instance lib_cell
```

The `replace_cell` command changes the cell of an instance. The replacement cell must have the same port list (number, name, and order) as the instance's existing cell for the replacement to be successful.

## report_activity_annotation

```
report_activity_annotation [-report_unannotated]  [-report_annotated]
```

Report a summary of pins that are annotated by `read_vcd`, `read_saif` or `set_power_activity`. Sequential internal pins and hierarchical pins are ignored.

### Options

`-report_unannotated`
: Report unannotated pins.

`-report_annotated`
: Report annotated pins.

## report_annotated_check

```
report_annotated_check [-setup] [-hold] [-recovery] [-removal] [-nochange] [-width] [-period] [-max_skew] [-scene scene] [-max_lines lines] [-report_annotated] [-report_unannotated] [-constant_arcs]
```

The `report_annotated_check` command reports a summary of SDF timing check annotation. The `-report_annotated` and `-report_annotated` options can be used to list arcs that are annotated or not annotated.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-recovery`
: Report annotated recovery checks.

`-removal`
: Report annotated removal checks.

`-nochange`
: Report annotated nochange checks.

`-width`
: Report annotated width checks.

`-period`
: Report annotated period checks.

`-max_skew`
: Report annotated max skew checks.

`-scene`
: Restrict the command to one scene.

`-max_lines`
: `lines`: Maximum number of lines listed by the `-report_annotated` and `-report_unannotated` options.

`-report_annotated`
: Report annotated timing arcs.

`-report_unannotated`
: Report unannotated timing arcs.

`-constant_arcs`
: Report separate annotation counts for arcs disabled by logic constants (`set_logic_one`, `set_logic_zero`).

## report_annotated_delay

```
report_annotated_delay [-cell] [-net] [-from_in_ports] [-to_out_ports] [-scene scene] [-max_lines lines] [-report_annotated] [-report_unannotated] [-constant_arcs]
```

The `report_annotated_delay` command reports a summary of SDF delay annotation. Without the `-from_in_ports` and `-to_out_ports` options arcs to and from top level ports are not reported. The `-report_annotated` and `-report_unannotated` options can be used to list arcs that are annotated or not annotated.

### Options

`-cell`
: Report annotated cell delays.

`-net`
: Report annotated internal net delays.

`-from_in_ports`
: Report annotated delays from input ports.

`-to_out_ports`
: Report annotated delays to output ports.

`-scene`
: Restrict the command to one scene.

`-max_lines`
: `lines`: Maximum number of lines listed by the `-report_annotated` and `-report_unannotated` options.

`-report_annotated`
: Report annotated timing arcs.

`-report_unannotated`
: Report unannotated timing arcs.

`-constant_arcs`
: Report separate annotation counts for arcs disabled by logic constants (`set_logic_one`, `set_logic_zero`).

## report_arrival

```
report_arrival [-scene scene] [-report_variance] [-digits digits] pin
```

The `report_arrival` command reports min/max rise/fall arrival times at a pin with respect to each clock that has a path to the pin.

### Options

`-scene`
: Restrict the command to one scene.

`-report_variance`
: Include delay distribution variance in the report.

`-digits`
: Number of digits to print after the decimal point.

## report_check_types

```
report_check_types [-scenes scenes] [-violators] [-verbose] [-format slack_only|end] [-max_delay] [-min_delay] [-recovery] [-removal] [-clock_gating_setup] [-clock_gating_hold] [-max_slew] [-min_slew] [-max_fanout] [-min_fanout] [-max_capacitance] [-min_capacitance] [-min_pulse_width] [-min_period] [-max_skew] [-net net] [-max_count max_count] [-digits digits] [-no_line_splits] [> filename] [>> filename]
```

The `report_check_types` command reports the slack for each type of timing and design rule constraint. The keyword options allow a subset of the constraint types to be reported.

### Options

`-scenes`
: Report checks for some scenes. The default value is all scenes.

`-violators`
: Report all violated timing and design rule constraints.

`-verbose`
: Use a verbose output format.

`-format`
: - `slack_only`: Report the minimum slack for each timing check.
  - `end`: Report the endpoint for each check.

`-max_delay`
: Report setup and max delay path delay constraints.

`-min_delay`
: Report hold and min delay path delay constraints.

`-recovery`
: Report asynchronous recovery checks.

`-removal`
: Report asynchronous removal checks.

`-clock_gating_setup`
: Report gated clock enable setup checks.

`-clock_gating_hold`
: Report gated clock hold setup checks.

`-max_slew`
: Report max transition design rule checks.

`-min_slew`
: Report min slew design rule checks.

`-max_fanout`
: Report max fanout design rule checks.

`-min_fanout`
: Report min fanout design rule checks.

`-max_capacitance`
: Report max capacitance design rule checks.

`-min_capacitance`
: Report min capacitance design rule checks.

`-min_pulse_width`
: Report min pulse width design rule checks.

`-min_period`
: Report min period design rule checks.

`-max_skew`
: Report max skew design rule checks.

`-net`
: Report checks on this net.

`-max_count`
: Maximum number of checks to report.

`-digits`
: Number of digits to print after the decimal point.

`-no_line_splits`
: Do not split long lines into multiple lines.

## report_checks

```
report_checks [-from from_list|-rise_from from_list|-fall_from from_list] [-through through_list|-rise_through through_list|-fall_through through_list] [-to to_list|-rise_to to_list|-fall_to to_list] [-unconstrained] [-path_delay min|min_rise|min_fall|max|max_rise|max_fall|min_max] [-scenes scenes] [-group_path_count path_count]  [-endpoint_path_count path_count] [-unique_paths_to_endpoint] [-unique_edges_to_endpoint] [-slack_max slack_max] [-slack_min slack_min] [-sort_by_slack] [-path_group group_name] [-format full|full_clock|full_clock_expanded|short|end|slack_only|summary|json] [-fields capacitance|slew|fanout|input_pin|net|src_attr|variation] [-digits digits] [-no_line_splits] [> filename] [>> filename]
```

The `report_checks` command reports paths in the design. Paths are reported in groups by capture clock, unclocked path delays, gated clocks and unconstrained.

See `set_false_path` for a description of allowed from_list, through_list and to_list objects.

### Options

`-from`
: Report paths from a list of clocks, instances, ports, register clock pins, or latch data pins.

`-rise_from`
: Report  paths from the rising edge of clocks, instances, ports, register clock pins, or latch data pins.

`-fall_from`
: Report paths from the falling edge of clocks, instances, ports, register clock pins, or latch data pins.

`-through`
: Report paths through a list of instances, pins or nets.

`-rise_through`
: Report rising paths through a list of instances, pins or nets.

`-fall_through`
: Report falling paths through a list of instances, pins or nets.

`-to`
: Report paths to a list of clocks, instances, ports or pins.

`-rise_to`
: Report rising paths to a list of clocks, instances, ports or pins.

`-fall_to`
: Report falling paths to a list of clocks, instances, ports or pins.

`-unconstrained`
: Report unconstrained paths also. The unconstrained path group is not reported without this option.

`-path_delay`
: - `min`: Report min path (hold) checks.
  - `min_rise`: Report min path (hold) checks for rising endpoints.
  - `min_fall`: Report min path (hold) checks for falling endpoints.
  - `max`: Report max path (setup) checks.
  - `max_rise`: Report max path (setup) checks for rising endpoints.
  - `max_fall`: Report max path (setup) checks for falling endpoints.
  - `min_max`: Report min and max path (setup and hold) checks.

`-scenes`
: Report paths for these scenes. The default is all scenes.

`-group_path_count`
: `path_count`: The number of paths to report in each path group. The default is 1.

`-endpoint_path_count`
: `endpoint_path_count`: The number of paths to report for each endpoint. The default is 1.

`-unique_paths_to_endpoint`
: When multiple paths to an endpoint are specified with `-endpoint_path_count`, many of the paths may differ only in the rise/fall edges of the pins in the paths. With this option only the worst path through the set of pins is reported.

`-unique_edges_to_endpoint`
: When multiple paths to an endpoint are specified with `-endpoint_path_count`, conditional timing arcs result in paths that go through the same pins and rise/fall edges. With this option only the worst path through the set of pins and rise/fall edges is reported.

`-slack_max`
: Only report paths with less slack than max_slack.

`-slack_min`
: Only report paths with more slack than min_slack.

`-sort_by_slack`
: Sort paths by slack rather than slack grouped by path group.

`-path_group`
: List of path groups to report. The default is to report all path groups.

`-format`
: - `end`: Report path ends in one line with delay, required time and slack.
  - `full`: Report path start and end points and the path. This is the default path type.
  - `full_clock`: Report path start and end points, the path, and the source and target clock paths.
  - `full_clock_expanded`: Report path start and end points, the path, and the source and target clock paths. If the clock is generated and propagated, the path from the clock source pin is also reported.
  - `short`: Report only path start and end points.
  - `summary`: Report only path ends with delay.
  - `json`: Report in json format. `-fields` is ignored.

`-fields`
: List of capacitance|slew|input_pins|hierarchical_pins|net|fanout|src_attr|variation

`-digits`
: Number of digits to print after the decimal point.

`-no_line_splits`
: Do not split long lines into multiple lines.

## report_clock_latency

```
report_clock_latency [-clocks clocks] [-scenes scene] [-include_internal_latency]
                                          [-digits digits]
```

Report the clock network latency.

### Options

`-clocks`
: The clocks to report. The default is all clocks.

`-scenes`
: Report latency for these scenes. The default is all scenes.

`-include_internal_latency`
: Include internal clock latency from liberty min/max_clock_tree_path timing groups.

`-digits`
: Number of digits to print after the decimal point.

## report_clock_min_period

```
report_clock_min_period [-clocks clocks] [-include_port_paths]
```

Report the minimum period and maximum frequency for clocks. If the `-clocks` argument is not specified all clocks are reported. The minimum period is determined by examining the smallest slack paths between registers on the rising edges of the clock or between falling edges of the clock. Paths between different clocks, different clock edges of the same clock, level-sensitive latches, or paths constrained by `set_multicycle_path` or `set_max_delay` are not considered.

### Options

`-clocks`
: The clocks to report.

`-include_port_paths`
: Include paths from input port and to output ports.

## report_clock_properties

```
report_clock_properties [clocks]
```

The `report_clock_properties` command reports the period and rise/fall edge times for each clock that has been defined.

## report_clock_skew

```
report_clock_skew [-setup|-hold] [-clocks clocks] [-scenes scenes] [-include_internal_latency]
                                       [-digits digits]
```

Report the maximum difference in clock arrival between every source and target register that has a path between the source and target registers.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-clocks`
: The clocks to report. The default is all clocks.

`-scenes`
: Report clocks for these scenes. The default is all scenes.

`-include_internal_latency`
: Include internal clock latency from liberty min/max_clock_tree_path timing groups.

`-digits`
: Number of digits to print after the decimal point.

## report_dcalc

```
report_dcalc [-from from_pin] [-to to_pin] [-scene scene] [-min] [-max] [-digits digits]
```

The `report_dcalc` command shows how the delays between instance pins are calculated. It is useful for debugging problems with delay calculation.

### Options

`-from`
: Report delay calculations for timing arcs from instance input pin from_pin.

`-to`
: Report delay calculations for timing arcs to instance output pin to_pin.

`-scene`
: Report delay calculations for this scene. Required if more than one scene is defined.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-digits`
: Number of digits to print after the decimal point.

## report_disabled_edges

```
report_disabled_edges
```

The `report_disabled_edges` command reports disabled timing arcs along with the reason they are disabled. Each disabled timing arc is reported as the instance name along with the from and to ports of the arc. The disable reason is shown next. Arcs that are disabled with `set_disable_timing` are reported with constraint as the reason. Arcs that are disabled by constants are reported with constant as the reason along with the constant instance pin and value. Arcs that are disabled to break combinational feedback loops are reported with loop as the reason.

```
> report_disabled_edges
u1 A B constant B=0
```

## report_edges

```
report_edges [-from from_pin] [-to to_pin] [-digits digits] [-report_variance]
```

Report the edges/timing arcs and their delays in the timing graph from/to/between pins.

### Options

`-from`
: Report edges/timing arcs from pin from_pin.

`-to`
: Report edges/timing arcs to pin to_pin.

`-digits`
: Number of digits to print after the decimal point.

`-report_variance`
: Include delay distribution variance in the report.

## report_instance

```
report_instance [-connections] [-verbose] instance_path [> filename] [>> filename]
```

Report information about an instance.

### Options

`-connections`
: Deprecated; connections are always reported.

`-verbose`
: Deprecated; verbose output is always used.

## report_lib_cell

```
report_lib_cell cell_name [> filename] [>> filename]
```

Describe the liberty library cell cell_name.

## report_net

```
report_net [-scene scene] [-digits digits] net_path [> filename] [>> filename]
```

Report the connections and capacitance of a net.

### Options

`-scene`
: Restrict the command to one scene.

`-digits`
: Number of digits to print after the decimal point.

## report_object_full_names

```
report_object_full_names objects
```

The `report_object_full_names` command prints the hierarchical name of each object, sorted by full name.

## report_object_names

```
report_object_names objects
```

The `report_object_names` command prints the name of each object, sorted by name.

## report_parasitic_annotation

```
report_parasitic_annotation [-name spef_name] [-report_unannotated]
```

Report SPEF parasitic annotation completeness.

### Options

`-name`
: SPEF annotation name from `read_spef -name`.

`-report_unannotated`
: Report unannotated and partially annotated nets.

## report_power

```
report_power [-instances instances] [-highest_power_instances count] [-scene scene] [-digits digits] [-format format] [> filename] [>> filename]
```

The `report_power` command uses static power analysis based on propagated or annotated pin activities in the circuit using Liberty power models. The internal, switching, leakage and total power are reported. Design power is reported separately for combinational, sequential, macro and pad groups. Power values are reported in watts.

The `read_vcd` or `read_saif` commands can be used to read activities from a file based on simulation. If no simulation activities are available, the `set_power_activity` command should be used to set the activity of input ports or pins in the design. The default input activity and duty for inputs are 0.1 and 0.5 respectively. The activities are propagated from annotated input ports or pins through gates and used in the power calculations.

```
Group                  Internal  Switching    Leakage      Total
                          Power      Power      Power      Power
----------------------------------------------------------------
Sequential             3.29e-06   3.41e-08   2.37e-07   3.56e-06  92.4%
Combinational          1.86e-07   3.31e-08   7.51e-08   2.94e-07   7.6%
Macro                  0.00e+00   0.00e+00   0.00e+00   0.00e+00   0.0%
Pad                    0.00e+00   0.00e+00   0.00e+00   0.00e+00   0.0%
---------------------------------------------------------------
Total                  3.48e-06   6.72e-08   3.12e-07   3.86e-06 100.0%
                          90.2%       1.7%       8.1%
```

### Options

`-instances`
: `instances`: Report the power for each instance of instances. If the instance is hierarchical the total power for the instances inside the hierarchical instance is reported.

`-highest_power_instances`
: `count`: Report the power for the count highest power instances.

`-scene`
: Restrict the command to one scene.

`-digits`
: Number of digits to print after the decimal point.

`-format`
: - `text`: Print a text table (the default).
  - `json`: Print JSON.

## report_required

```
report_required [-scene scene] [-report_variance] [-digits digits] pin
```

The `report_required` command reports min/max rise/fall required times at a pin with respect to each clock.

### Options

`-scene`
: Restrict the command to one scene.

`-report_variance`
: Include delay distribution variance in the report.

`-digits`
: Number of digits to print after the decimal point.

## report_slack

```
report_slack [-scene scene] [-report_variance] [-digits digits] pin
```

The `report_slack` command reports min/max rise/fall slack at a pin with respect to each clock.

### Options

`-scene`
: Restrict the command to one scene.

`-report_variance`
: Include delay distribution variance in the report.

`-digits`
: Number of digits to print after the decimal point.

## report_slews

```
report_slews [-scenes scenes] [-digits digits] [-report_variance] pin
```

Report the slews at a pin.

### Options

`-scenes`
: Report slews for these scenes. The default is all scenes.

`-digits`
: Number of digits to print after the decimal point.

`-report_variance`
: Report SSTA distribution parameters.

## report_tns

```
report_tns [-min] [-max] [-digits digits]
```

Report the total negative slack.

### Options

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-digits`
: Number of digits to print after the decimal point.

## report_units

```
report_units
```

Report the units used for command arguments and reporting.

```
report_units
 time 1ns
 capacitance 1pF
 resistance 1kohm
 voltage 1v
 current 1A
 power 1pW
 distance 1um
```

## report_wns

```
report_wns [-min] [-max] [-digits digits]
```

Report the worst negative slack. If the worst slack is positive, zero is reported.

### Options

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-digits`
: Number of digits to print after the decimal point.

## report_worst_slack

```
report_worst_slack [-min] [-max] [-digits digits]
```

Report the worst slack in the design.

### Options

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-digits`
: Number of digits to print after the decimal point.

## set_assigned_check

```
set_assigned_check -setup|-hold|-recovery|-removal [-rise] [-fall] [-scene scene] [-min] [-max] [-from from_pins] [-to to_pins] [-clock rise|fall] [-cond sdf_cond] check_value
```

The `set_assigned_check` command is used to annotate the timing checks between two pins on an instance. The annotated delay overrides the calculated delay. This command is an interactive way to back-annotate delays like an SDF file.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-recovery`
: Annotate recovery timing checks.

`-removal`
: Annotate removal timing checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-scene`
: The name of a scene. The `-scene` keyword is required if more than one scene  is defined.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-from`
: A list of pins for the clock.

`-to`
: A list of pins for the data.

`-clock`
: `rise|fall`: The timing check clock pin transition.

`-cond`
: SDF condition string for the annotated check.

## set_assigned_delay

```
set_assigned_delay -cell|-net [-rise] [-fall] [-scene scene] [-min] [-max] [-from from_pins] [-to to_pins] delay
```

The `set_assigned_delay` command is used to annotate the delays between two pins on an instance or net. The annotated delay overrides the calculated delay. This command is an interactive way to back-annotate delays like an SDF file.

### Options

`-cell`
: Annotate the delays between two pins on an instance.

`-net`
: Annotate the delays between two pins on a net.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-scene`
: The name of a scene. The `-scene` keyword is required if more than one scene is defined.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-from`
: A list of pins.

`-to`
: A list of pins.

## set_assigned_transition

```
set_assigned_transition [-rise] [-fall] [-scene scene] [-min] [-max] slew pins
```

The `set_assigned_transition` command is used to annotate the transition time (slew) of a pin. The annotated transition time overrides the calculated transition time.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-scene`
: Annotate delays for scene.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_case_analysis

```
set_case_analysis 0|1|zero|one|rise|rising|fall|falling pins
```

The `set_case_analysis` command sets the signal on a port or pin to a constant logic value. No paths are propagated from constant pins. Constant values set with the `set_case_analysis` command are propagated through downstream gates.

Conditional timing arcs with mode groups are controlled by logic values on the instance pins.

## set_clock_gating_check

```
set_clock_gating_check [-setup setup_time] [-hold hold_time] [-rise] [-fall] [-low] [-high] [objects]
```

The `set_clock_gating_check` command is used to add setup or hold timing checks for data signals used to gate clocks.

If no objects are specified the setup/hold margin is global and applies to all clock gating circuits in the design. If neither of the `-rise` and `-fall` options are used the setup/hold margin applies to the rising and falling  edges of the clock gating signal.

Normally the library cell function is used to determine the active state of the clock. The clock is active high for AND/NAND functions and active low for OR/NOR functions. The `-high` and `-low` options are used to specify the active state of the clock for other cells, such as a MUX.

If multiple `set_clock_gating_check` commands apply to a clock gating instance he priority of the commands is shown below (highest to lowest priority).

```
clock enable pin
instance
clock pin
clock
global
```

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-low`
: The gating clock is active low (pin and instance objects only).

`-high`
: The gating clock is active high (pin and instance objects only).

## set_clock_groups

```
set_clock_groups [-name name] [-logically_exclusive] [-physically_exclusive] [-asynchronous] [-allow_paths] [-comment comment] -group clocks
```

The `set_clock_groups` command is used to define groups of clocks that interact with each other. Clocks in different groups do not interact and paths between them are not reported. Use a `-group` argument for each clock group.

### Options

`-name`
: `name`: The clock group name.

`-logically_exclusive`
: The clocks in different groups do not interact logically but can be physically present on the same chip. Paths between clock groups are considered for noise analysis.

`-physically_exclusive`
: The clocks in different groups cannot be present at the same time on a chip. Paths between clock groups are not considered for noise analysis.

`-asynchronous`
: The clock groups are asynchronous. Paths between clock groups are considered for noise analysis.

`-allow_paths`
: Allow paths between clock groups (do not mark them as false).

`-comment`
: Comment string saved with the constraint.

`-group`
: A list of clocks in one group. Repeat `-group` for each group.

## set_clock_latency

```
set_clock_latency [-source] [-clock clock] [-rise] [-fall] [-min] [-max] [-early] [-late] delay objects
```

The `set_clock_latency` command describes expected delays of the clock tree when analyzing a design using ideal clocks. Use the `-source` option to specify latency at the clock source, also known as insertion delay. Source latency is delay in the clock tree that is external to the design or a clock tree internal to an instance that implements a complex logic function.

`set_clock_latency` removes propagated clock properties for the clocks and pins objects.

### Options

`-source`
: The latency is at the clock source.

`-clock`
: `clock`: If multiple clocks are defined at a pin this use this option to specify the latency for a specific clock.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-early`
: Apply to early (min path) values.

`-late`
: Apply to late (max path) values.

## set_clock_sense

```
set_clock_sense [-positive] [-negative] [-pulse pulse_type] [-stop_propagation]  [-clock clocks] pins
```

The `set_clock_sense` command is deprecated as of SDC 2.1. Use `set_sense -type clock` instead.

### Options

`-positive`
: The clock sense is positive unate.

`-negative`
: The clock sense is negative unate.

`-pulse`
: Pulse type. Not supported.

`-stop_propagation`
: Stop propagating clocks at pins.

`-clock`
: A list of clocks to apply the sense.

## set_clock_transition

```
set_clock_transition [-rise] [-fall] [-min] [-max] transition clocks
```

The `set_clock_transition` command describes expected transition times of the clock tree when analyzing a design using ideal clocks.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_clock_uncertainty

```
set_clock_uncertainty [-from|-rise_from|-fall_from from_clock] [-to|-rise_to|-fall_to to_clock] [-rise] [-fall] [-setup] [-hold] uncertainty [objects]
```

The `set_clock_uncertainty` command specifies the uncertainty or jitter in a clock. The uncertainty for a clock can be specified on its source pin or port, or the clock itself.

```
set_clock_uncertainty .1 [get_clock clk1]
```

Inter-clock uncertainty between the source and target clocks of timing checks is specified with the `-from`|`-rise_from`|`-fall_from` and `-to`|`-rise_to`|`-fall_to` arguments .

```
set_clock_uncertainty -from [get_clock clk1] -to [get_clocks clk2] .1
```

The following commands are equivalent.

```
set_clock_uncertainty -from [get_clock clk1] -rise_to [get_clocks clk2] .1
set_clock_uncertainty -from [get_clock clk1] -to [get_clocks clk2] -rise .1
```

### Options

`-from`
: `from_clock`: Inter-clock uncertainty source clock.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-to`
: `to_clock`: Inter-clock uncertainty target clock.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

## set_cmd_units

```
set_cmd_units [-capacitance cap_unit] [-resistance res_unit] [-time time_unit] [-voltage voltage_unit] [-current current_unit] [-power power_unit] [-distance distance_unit]
```

The `set_cmd_units` command is used to change the units used by the STA command interpreter when parsing commands and reporting results. The default units are the units specified in the first Liberty library file that is read.

Units are specified as a scale factor followed by a unit name. The scale factors are as follows.

```
M 1E+6
k 1E+3
m 1E-3
u 1E-6
n 1E-9
p 1E-12
f 1E-15
```

An example of the `set_units` command is shown below.

```
set_cmd_units -time ns -capacitance pF -current mA -voltage V
              -resistance kOhm -distance um
```

### Options

`-capacitance`
: `cap_unit`: The capacitance scale factor followed by 'f'.

`-resistance`
: `res_unit`: The resistance scale factor followed by 'ohm'.

`-time`
: `time_unit`: The time scale factor followed by 's'.

`-voltage`
: `voltage_unit`: The voltage scale factor followed by 'v'.

`-current`
: `current_unit`: The current scale factor followed by 'A'.

`-power`
: `power_unit`: The power scale factor followed by 'w'.

`-distance`
: `distance_unit`: The distance scale factor followed by 'm'.

## set_data_check

```
set_data_check [-from from_pin] [-rise_from from_pin] [-fall_from from_pin] [-to to_pin] [-rise_to to_pin] [-fall_to to_pin] [-setup | -hold] [-clock clock] margin
```

The `set_data_check` command is used to add a setup or hold timing check between two pins.

### Options

`-from`
: `from_pin`: A pin used as the timing check reference.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-to`
: `to_pin`: A pin that the setup/hold check is applied to.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-clock`
: `clock`: The setup/hold check clock.

## set_disable_inferred_clock_gating

```
set_disable_inferred_clock_gating objects
```

The `set_disable_inferred_clock_gating` command disables clock gating checks on a clock gating instance, clock gating pin, or clock gating enable pin.

## set_disable_timing

```
set_disable_timing [-from from_port] [-to to_port] objects
```

The `set_disable_timing` command is used to disable paths though pins in the design. There are many different forms of the command depending on the objects specified in objects.

All timing paths though an instance are disabled when objects contains an instance. Timing checks in the instance are not disabled.

```
set_disable_timing u2
```

The `-from` and `-to` options can be used to restrict the disabled path to those from, to or between specific pins on the instance.

```
set_disable_timing -from A u2
set_disable_timing -to Z u2
set_disable_timing -from A -to Z u2
```

A list of top level ports or instance pins can also be disabled.

```
set_disable_timing u2/Z
set_disable_timing in1
```

Timing paths though all instances of a library cell in the design can be disabled by naming the cell using a hierarchy separator between the library and cell name. Paths from or to a cell port can be disabled with the `-from` and `-to` options or a port name after library and cell names.

```
set_disable_timing liberty1/snl_bufx2
set_disable_timing -from A liberty1/snl_bufx
set_disable_timing -to Z liberty1/snl_bufx
set_disable_timing liberty1/snl_bufx2/A
```

### Options

`-from`
: From pin of the disabled timing arc on an instance or cell.

`-to`
: To pin of the disabled timing arc on an instance or cell.

## set_drive

```
set_drive [-rise] [-fall] [-min] [-max]  resistance ports
```

The `set_drive` command describes the resistance of an input port external driver.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_driving_cell

```
set_driving_cell [-lib_cell cell] [-library library] [-rise] [-fall] [-min] [-max] [-pin pin] [-from_pin from_pin] [-input_transition_rise trans_rise] [-input_transition_fall trans_fall] [-multiply_by factor] [-dont_scale] [-no_design_rule] ports
```

The `set_driving_cell` command describes an input port external driver.

### Options

`-lib_cell`
: `cell_name`: The driving cell.

`-library`
: `library`: The driving cell library.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-pin`
: `pin`: The output port of the driving cell.

`-from_pin`
: `from_pin`: Use timing arcs from from_pin to the output pin.

`-input_transition_rise`
: `trans_rise`: The transition time for a rising input at from_pin.

`-input_transition_fall`
: `trans_fall`: The transition time for a falling input at from_pin.

`-multiply_by`
: Scale factor applied to the driving cell delay. Ignored.

`-dont_scale`
: Do not scale the driving cell delay. Ignored.

`-no_design_rule`
: Do not apply driving cell design rules. Ignored.

## set_false_path

```
set_false_path [-setup] [-hold] [-rise] [-fall] [-reset_path] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list]
```

The `set_false_path` command disables timing along a path from, through and to a group of design objects.

Objects in from_list can be clocks, register/latch instances, or register/latch clock pins. The `-rise_from` and `-fall_from` keywords restrict the false paths to a specific clock edge.

Objects in through_list can be nets, instances, instance pins, or hierarchical pins,. The `-rise_through` and `-fall_through` keywords restrict the false paths to a specific path edge that traverses through the object.

Objects in to_list can be clocks, register/latch instances, or register/latch clock pins. The `-rise_to` and `-fall_to` keywords restrict the false paths to a specific transition at the path end.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-reset_path`
: Remove any matching `set_false_path`, `set_multicycle_path`, `set_max_delay`, `set_min_delay` exceptions first.

`-comment`
: Comment string saved with the constraint.

`-from`
: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-through`
: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-to`
: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## set_fanout_load

```
set_fanout_load fanout ports
```

This command is ignored.

## set_hierarchy_separator

```
set_hierarchy_separator separator
```

Set the character used to separate names in a hierarchical instance, net or pin name. This separator is used by the command interpreter to read arguments and print results. The default separator is '/'.

## set_ideal_latency

```
set_ideal_latency [-rise] [-fall] [-min] [-max] delay objects
```

The `set_ideal_latency` command is parsed but ignored.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_ideal_network

```
set_ideal_network [-no_propagation] objects
```

The `set_ideal_network` command is parsed but ignored.

### Options

`-no_propagation`
: Do not propagate the ideal network. Ignored.

## set_ideal_transition

```
set_ideal_transition [-rise] [-fall] [-min] [-max] transition_time objects
```

The `set_ideal_transition` command is parsed but ignored.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_input_delay

```
set_input_delay [-rise] [-fall] [-max] [-min] [-clock clock] [-clock_fall] [-reference_pin ref_pin] [-source_latency_included] [-network_latency_included] [-add_delay] delay port_pin_list
```

The `set_input_delay` command is used to specify the arrival time of an input signal.

The following command sets the min, max, rise and fall times on the in1 input port 1.0 time units after the rising edge of clk1.

```
set_input_delay -clock clk1 1.0 [get_ports in1]
```

Use multiple commands with the `-add_delay` option to specify separate arrival times for min, max, rise and fall times or multiple clocks. For example, the following specifies separate arrival times with respect to clocks clk1 and clk2.

```
set_input_delay -clock clk1 1.0 [get_ports in1]
set_input_delay -add_delay -clock clk2 2.0 [get_ports in1]
```

The `-reference_pin` option is used to specify an arrival time with respect to the arrival on a pin in the clock network. For propagated clocks, the input arrival time is relative to the clock arrival time at the reference pin (the clock source latency and network latency from the clock source to the reference pin). For ideal clocks, input arrival time is relative to the reference pin clock source latency. With the `-clock_fall` flag the arrival time is relative to the falling transition at the reference pin. If no clocks arrive at the reference pin the `set_input_delay` command is ignored. If no `-clock` is specified the arrival time is with respect to all clocks that arrive at the reference pin. The `-source_latency_included` and `-network_latency_included` options cannot be used with `-reference_pin`.

Paths from inputs that do not have an arrival time defined by `set_input_delay` are not reported. Set the `sta_input_port_default_clock` variable to 1 to report paths from inputs without a `set_input_delay`.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

`-clock`
: `clock`: The arrival time is from clock.

`-clock_fall`
: The arrival time is from the falling edge of clock.

`-reference_pin`
: `ref_pin`: The arrival time is with respect to the clock that arrives at ref_pin.

`-source_latency_included`
: D no add the clock source latency (insertion delay) to the delay value.

`-network_latency_included`
: Do not add the clock latency to the delay value when the clock is ideal.

`-add_delay`
: Add this arrival to any existing arrivals.

## set_input_transition

```
set_input_transition [-rise] [-fall] [-min] [-max] transition ports
```

The `set_input_transition` command is used to specify the transition time (slew) of an input signal.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_level_shifter_strategy

```
set_level_shifter_strategy [-rule rule_type]
```

This command is parsed and ignored by timing analysis.

### Options

`-rule`
: Level shifter rule. Ignored.

## set_level_shifter_threshold

```
set_level_shifter_threshold [-voltage volt]
```

This command is parsed and ignored by timing analysis.

### Options

`-voltage`
: Voltage threshold. Ignored.

## set_load

```
set_load [-rise] [-fall] [-max] [-min] [-subtract_pin_load] [-pin_load] [-wire_load] capacitance objects
```

The `set_load` command annotates wire capacitance on a net or external capacitance on a port. There are four different uses for the `set_load` commanc:

```
set_load -wire_load port  external port wire capacitance
set_load -pin_load port   external port pin capacitance
set_load port             same as -pin_load
set_load net              net wire capacitance
```

External port capacitance can be annotated separately with the `-pin_load` and `-wire_load` options. Without the `-pin_load` and `-wire_load` options pin capacitance is annotated.

When annotating net wire capacitance with the `-subtract_pin_load` option the capacitance of all instance pins connected to the net is subtracted from capacitance. Setting the capacitance on a net overrides SPEF parasitics for delay calculation.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

`-subtract_pin_load`
: Subtract the capacitance of all instance pins connected to the net from capacitance (nets only). If the resulting capacitance is negative, zero is used. Pin capacitances are ignored by delay calculation when this option is used.

`-pin_load`
: capacitance is external instance pin capacitance (ports only).

`-wire_load`
: capacitance is external wire capacitance (ports only).

## set_logic_dc

```
set_logic_dc port_list
```

Set a port or pin to a constant unknown logic value. No paths are propagated from constant pins.

## set_logic_one

```
set_logic_one port_list
```

Set a port or pin to a constant logic one value. No paths are propagated from constant pins. Constant values set with the `set_logic_one` command are not propagated through downstream gates.

## set_logic_zero

```
set_logic_zero port_list
```

Set a port or pin to a constant logic zero value. No paths are propagated from constant pins. Constant values set with the `set_logic_zero` command are not propagated through downstream gates.

## set_max_area

```
set_max_area area
```

The `set_max_area` command is ignored during timing but is included in SDC files that are written.

## set_max_capacitance

```
set_max_capacitance cap objects
```

The `set_max_capacitance` command is ignored during timing but is included in SDC files that are written.

## set_max_delay

```
set_max_delay [-rise] [-fall] [-ignore_clock_latency] [-reset_path] [-probe] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list] delay
```

The `set_max_delay` command constrains the maximum delay through combinational logic paths. See `set_false_path` for a description of allowed from_list, through_list and to_list objects. If the to_list ends at a timing check the setup/hold time is included in the path delay.

When the `-ignore_clock_latency` option is used clock latency at the source and destination of the path delay is ignored. The constraint is reported in the default path group (**default**) rather than the clock path group when the path ends at a timing check.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-ignore_clock_latency`
: Ignore clock latency at the source and target registers.

`-reset_path`
: Remove any matching `set_false_path`, `set_multicycle_path`, `set_max_delay`, `set_min_delay` exceptions first.

`-probe`
: Do not break paths at internal pins (non startpoints).

`-comment`
: Comment string saved with the constraint.

`-from`
: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-through`
: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-to`
: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## set_max_dynamic_power

```
set_max_dynamic_power power [unit]
```

The `set_max_dynamic_power` command is ignored during timing but is included in SDC files that are written.

## set_max_fanout

```
set_max_fanout fanout objects
```

The `set_max_fanout` command is ignored during timing but is included in SDC files that are written.

## set_max_leakage_power

```
set_max_leakage_power power [unit]
```

The `set_max_leakage_power` command is ignored during timing but is included in SDC files that are written.

## set_max_time_borrow

```
set_max_time_borrow limit objects
```

The `set_max_time_borrow` command specifies the maximum amount of time that latches can borrow. Time borrowing is the time that a data input to a transparent latch arrives after the latch opens.

## set_max_transition

```
set_max_transition [-clock_path] [-data_path] [-rise] [-fall] slew objects
```

The `set_max_transition` command is specifies the maximum transition time (slew) design rule checked by the `report_check_types` `-max_transition` command.

If specified for a design, the default maximum transition is set for the design.

If specified for a clock, the maximum transition is applied to all pins in the clock domain. The `-clock_path` option restricts the maximum transition to clocks in clock paths. The `-data_path` option restricts the maximum transition to clocks data paths. The `-clock_path`, `-data_path`, `-rise` and `-fall` options only apply to clock objects.

### Options

`-clock_path`
: Set the  max slew for clock paths.

`-data_path`
: Set the  max slew for data paths.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

## set_min_capacitance

```
set_min_capacitance cap objects
```

The `set_min_capacitance` command is ignored during timing but is included in SDC files that are written.

## set_min_delay

```
set_min_delay [-rise] [-fall] [-ignore_clock_latency] [-reset_path] [-probe] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list] delay
```

The `set_min_delay` command constrains the minimum delay through combinational logic. See `set_false_path` for a description of allowed from_list, through_list and to_list objects. If the to_list ends at a timing check the setup/hold time is included in the path delay.

When the `-ignore_clock_latency` option is used clock latency at the source and destination of the path delay is ignored. The constraint is reported in the default path group (**default**) rather than the clock path group when the path ends at a timing check.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-ignore_clock_latency`
: Ignore clock latency at the source and target registers.

`-reset_path`
: Remove any matching `set_false_path`, `set_multicycle_path`, `set_max_delay`, `set_min_delay` exceptions first.

`-probe`
: Do not break paths at internal pins (non startpoints).

`-comment`
: Comment string saved with the constraint.

`-from`
: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-through`
: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-to`
: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## set_min_pulse_width

```
set_min_pulse_width [-low] [-high] value [objects]
```

If `-low` and `-high` are not specified the minimum width applies to both high and low pulses.

### Options

`-low`
: Set the minimum low pulse width.

`-high`
: Set the minimum high pulse width.

## set_mode

```
set_mode mode_name
```

Set the mode for SDC commands in the Tcl interpreter. If mode `mode_name` does not exist, it is created. When modes are created the default mode is deleted.

## set_multicycle_path

```
set_multicycle_path [-setup] [-hold] [-rise] [-fall] [-start] [-end] [-reset_path] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list] path_multiplier
```

Normally the path between two registers or latches is assumed to take one clock cycle. The `set_multicycle_path` command overrides this assumption and allows multiple clock cycles for a timing check. See `set_false_path` for a description of allowed from_list, through_list and to_list objects.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-start`
: Multiply the source clock period by period_multiplier.

`-end`
: Multiply the target clock period by period_multiplier.

`-reset_path`
: Remove any matching `set_false_path`, `set_multicycle_path`, `set_max_delay`, `set_min_delay` exceptions first.

`-comment`
: Comment string saved with the constraint.

`-from`
: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-through`
: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-to`
: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## set_operating_conditions

```
set_operating_conditions [-analysis_type single|bc_wc|on_chip_variation] [-library lib] [condition] [-min min_condition] [-max max_condition] [-min_library min_lib] [-max_library max_lib]
```

The `set_operating_conditions` command is used to specify the type of analysis performed and the operating conditions used to derate library data.

### Options

`-analysis_type`
: - `single`: Use one operating condition for min and max paths.
  - `bc_wc`: Best case, worst case analysis. Setup checks use max_condition for clock and data paths. Hold checks use the min_condition for clock and data paths.
  - `on_chip_variation`: The min and max operating conditions represent variations on the chip that can occur simultaneously. Setup checks use max_condition for data paths and    min_condition for clock paths. Hold checks use min_condition for data paths and max_condition for clock paths. This is the default analysis type.

`-library`
: `lib`: The name of the library that contains condition.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-min_library`
: `min_lib`: The name of the library that contains min_condition.

`-max_library`
: `max_lib`: The name of the library that contains max_condition.

## set_output_delay

```
set_output_delay [-rise] [-fall] [-max] [-min] [-clock clock] [-clock_fall] [-reference_pin ref_pin] [-source_latency_included] [-network_latency_included] [-add_delay] delay port_pin_list
```

The `set_output_delay` command is used to specify the external delay to a setup/hold check on an output port or internal pin that is clocked by clock. Unless the `-add_delay` option is specified any existing output delays are replaced.

The `-reference_pin` option is used to specify a timing check with respect to the arrival on a pin in the clock network. For propagated clocks, the timing check is relative to the clock arrival time at the reference pin (the clock source latency and network latency from the clock source to the reference pin). For ideal clocks, the timing check is relative to the reference pin clock source latency. With the `-clock_fall` flag the timing check is relative to the falling edge of the reference pin. If no clocks arrive at the reference pin the `set_output_delay` command is ignored. If no `-clock` is specified the timing check is with respect to all clocks that arrive at the reference pin. The `-source_latency_included` and `-network_latency_included` options cannot be used with `-reference_pin`.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

`-clock`
: `clock`: The external check is to clock. The default clock edge is rising.

`-clock_fall`
: The external check is to the falling edge of clock.

`-reference_pin`
: `ref_pin`: The external check is clocked by the clock that arrives at ref_pin.

`-source_latency_included`
: Do not add the clock source latency (insertion delay) to the delay value.

`-network_latency_included`
: Do not add the clock latency to the delay value when the clock is ideal.

`-add_delay`
: Add this output delay to any existing output delays.

## set_path_margin

```
set_path_margin [-setup] [-hold] [-rise] [-fall] [-comment comment] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through|-thr|-th through_list] [-rise_through|-rise_thr|-rise_th through_list] [-fall_through|-fall_thr|-fall_th through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list] margin
```

The `set_path_margin` command applies a signed slack adjustment to matching timing paths on the capture-clock side. A positive margin makes the path harder to meet and a negative margin makes it easier. If neither `-setup` nor `-hold` is specified the margin applies to both. See `set_false_path` for a description of allowed from_list, through_list and to_list objects. At least one of `-from`, `-through`, or `-to` is required. Matching exceptions are removed with `unset_path_exceptions`.

### Options

`-through`
: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-comment`
: Comment string saved with the constraint.

`-from`
: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-to`
: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## set_port_fanout_number

```
set_port_fanout_number [-max] [-min] fanout ports
```

Set the external fanout for ports.

### Options

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

## set_power_activity

```
set_power_activity [-global] [-input] [-input_ports ports] [-pins pins] [-activity activity | -density density] [-duty duty] [-clock clock]
```

The `set_power_activity` command is used to set the activity and duty used for power analysis globally or for input ports or pins in the design.

The default input activity for inputs is 0.1 transitions per minimum clock period if a clock is defined or 0.0 if there are no clocks defined. The default input duty is 0.5. This is equivalent to the following command:

```
set_power_activity -input -activity 0.1 -duty 0.5
```

### Options

`-global`
: Set the activity/duty for all non-clock pins.

`-input`
: Set the default input port activity/duty.

`-input_ports`
: `input_ports`: Set the input port activity/duty.

`-pins`
: `pins`: Set the pin activity/duty.

`-activity`
: `activity`: The activity, or number of transitions per clock cycle. If clock is not specified the clock with the minimum period is used. If no clocks are defined an error is reported.

`-density`
: `density`: Transitions per library time unit.

`-duty`
: `duty`: The duty, or probability the signal is high (0 <= duty <= 1.0). Defaults to 0.5.

`-clock`
: `clock`: The clock to use for the period with `-activity`. This option is ignored if `-density` is used.

## set_propagated_clock

```
set_propagated_clock objects
```

The `set_propagated_clock` command changes a clock tree from an ideal network that has no delay one that uses calculated or back-annotated gate and interconnect delays. When objects is a port or pin, clock delays downstream of the object are used.

## set_property

```
set_property object property value
```

The `set_property` command sets a user property defined with `define_property` on an object. Use `get_property` to read the value.

## set_pvt

```
set_pvt insts [-min] [-max] [-process process] [-voltage voltage] [-temperature temperature]
```

The `set_pvt` command sets the process, voltage and temperature values used during delay calculation for a specific instance in the design.

### Options

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

`-process`
: `process`: A process value (float).

`-voltage`
: `voltage`: A voltage value (float).

`-temperature`
: `temperature`: A temperature value (float).

## set_resistance

```
set_resistance [-min] [-max] resistance nets
```

Set the resistance of nets.

### Options

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_scene

```
set_scene scene_name
```

The `set_scene` command sets the scene used by subsequent commands. Use `get_scenes` to find defined scenes.

## set_sense

```
set_sense [-type clock|data] [-positive] [-negative] [-pulse pulse_type] [-stop_propagation] [-clocks clocks] pins
```

The `set_sense` command is used to modify the propagation of a clock signal. The clock sense is set with the `-positive` and `-negative` flags. Use the `-stop_propagation` flag to stop the clock from propagating beyond a pin. The `-positive`, `-negative`, `-stop_propagation`, and `-pulse` options are mutually exclusive. If the `-clocks` option is not used the command applies to all clocks that traverse pins. The `-pulse` option is currently not supported.

### Options

`-type`
: - `clock`: Set the sense for clock paths.
  - `data`: Set the sense for data paths (not supported).

`-positive`
: The clock sense is positive unate.

`-negative`
: The clock sense is negative unate.

`-pulse`
: `pulse_type`: rise_triggered_high_pulse
  rise_triggered_low_pulse
  fall_triggered_high_pulse
  fall_triggered_low_pulse
  Not supported.

`-stop_propagation`
: Stop propagating clocks at pins.

`-clocks`
: A list of clocks to apply the sense.

## set_timing_derate

```
set_timing_derate -early|-late [-rise] [-fall] [-clock] [-data]  [-net_delay] [-cell_delay] [-cell_check] derate [objects]
```

The `set_timing_derate` command is used to derate delay calculation results used by the STA. If the `-early` and `-late` flags are omitted the both min and max paths are derated. If the `-clock` and `-data` flags are not used the derating both clock and data paths are derated.

Use the `unset_timing_derate` command to remove all derating factors.

### Options

`-early`
: Derate early (min) paths.

`-late`
: Derate late (max) paths.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-clock`
: Derate paths in the clock network.

`-data`
: Derate data paths.

`-net_delay`
: Derate net (interconnect) delays.

`-cell_delay`
: Derate cell delays.

`-cell_check`
: Derate cell timing check margins.

## set_units

```
set_units [-time time_unit] [-capacitance cap_unit] [-resistance res_unit] [-voltage voltage_unit] [-current current_unit] [-power power_unit] [-distance distance_unit]
```

The `set_units` command is used to check the units used by the STA command interpreter when parsing commands and reporting results. If the current units differ from the set_unit value a warning is printed. Use the `set_cmd_units` command to change the command units.

Units are specified as a scale factor followed by a unit name. The scale factors are as follows.

M 1E+6
k 1E+3
m 1E-3
u 1E-6
n 1E-9
p 1E-12
f 1E-15

An example of the `set_units` command is shown below.

`set_units` `-time` ns `-capacitance` pF `-current` mA `-voltage` V `-resistance` kOhm

### Options

`-time`
: `time_unit`: The time scale factor followed by 's'.

`-capacitance`
: `cap_unit`: The capacitance scale factor followed by 'f'.

`-resistance`
: `res_unit`: The resistance scale factor followed by 'ohm'.

`-voltage`
: `voltage_unit`: The voltage scale factor followed by 'v'.

`-current`
: `current_unit`: The current scale factor followed by 'A'.

`-power`
: `power_unit`: The power scale factor followed by 'w'.

`-distance`
: `distance_unit`: The distance scale factor followed by 'm'.

## set_voltage

```
set_voltage [-min min_case_value] [-object_list power_nets] max_case_voltage
```

The `set_voltage` command sets the supply voltage used by SDC. The max-case voltage is always set globally. If `-object_list` is given, it is also set on those power nets.

### Options

`-min`
: Minimum (min delay) voltage. If omitted, only the max-case voltage is set.

`-object_list`
: Power nets to apply the voltage to.

## set_wire_load_min_block_size

```
set_wire_load_min_block_size block_size
```

The `set_wire_load_min_block_size` command is not supported.

## set_wire_load_mode

```
set_wire_load_mode top|enclosed|segmented
```

The `set_wire_load_mode` command is ignored during timing but is included in SDC files that are written.

## set_wire_load_model

```
set_wire_load_model -name model_name [-library lib_name] [-min] [-max] [objects]
```

Set the wire load model used to estimate net parasitics.

### Options

`-name`
: `model_name`: The name of a wire load model.

`-library`
: `library`: Library to look for model_name.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## set_wire_load_selection_group

```
set_wire_load_selection_group [-library lib] [-min] [-max] group_name [objects]
```

The `set_wire_load_selection_group` command is parsed but not supported.

### Options

`-library`
: Library to look for group_name.

`-min`
: Apply to minimum (hold) analysis.

`-max`
: Apply to maximum (setup) analysis.

## suppress_msg

```
suppress_msg msg_ids
```

The `suppress_msg` command suppresses specified error/warning messages by ID. The list of message IDs can be found in doc/messages.md.

## unset_case_analysis

```
unset_case_analysis pins
```

The `unset_case_analysis` command removes the constant values defined by the `set_case_analysis` command.

## unset_clock_groups

```
unset_clock_groups [-logically_exclusive] [-physically_exclusive] [-asynchronous] [-name names] [-all]
```

The `unset_clock_groups` command removes clock groups defined with `set_clock_groups`. One of `-logically_exclusive`, `-physically_exclusive`, or `-asynchronous` is required. Use `-all` to remove every group of that type, or `-name` to remove named groups.

### Options

`-logically_exclusive`
: Remove logically exclusive clock groups.

`-physically_exclusive`
: Remove physically exclusive clock groups.

`-asynchronous`
: Remove asynchronous clock groups.

`-name`
: Names of clock groups to remove.

`-all`
: Remove all clock groups of the specified type.

## unset_clock_latency

```
unset_clock_latency [-source] [-clock clock] objects
```

The `unset_clock_latency` command removes the clock latency set with the `set_clock_latency` command.

### Options

`-source`
: Specifies source clock latency (clock insertion delay).

`-clock`
: If multiple clocks are defined at a pin, specify which clock latency to remove.

## unset_clock_transition

```
unset_clock_transition clocks
```

The `unset_clock_transition` command removes the clock transition set with the `set_clock_transition` command.

## unset_clock_uncertainty

```
unset_clock_uncertainty [-from|-rise_from|-fall_from from_clock] [-to|-rise_to|-fall_to to_clock] [-rise] [-fall] [-setup] [-hold] [objects]
```

The `unset_clock_uncertainty` command removes clock uncertainty defined with the `set_clock_uncertainty` command.

### Options

`-from`
: `from_clock`: Inter-clock uncertainty source clock.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-to`
: `to_clock`: Inter-clock uncertainty target clock.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

## unset_data_check

```
unset_data_check [-from from_pin] [-rise_from from_pin] [-fall_from from_pin] [-to to_pin] [-rise_to to_pin] [-fall_to to_pin] [-setup | -hold] [-clock clock]
```

The `unset_clock_transition` command removes a setup or hold check defined by the `set_data_check` command.

### Options

`-from`
: `from_object`: A pin used as the timing check reference.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-to`
: `to_object`: A pin that the setup/hold check is applied to.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-clock`
: The setup/hold check clock.

## unset_disable_inferred_clock_gating

```
unset_disable_inferred_clock_gating objects
```

The `unset_disable_inferred_clock_gating` command removes a previous `set_disable_inferred_clock_gating` command.

## unset_disable_timing

```
unset_disable_timing [-from from_port] [-to to_port] objects
```

The `unset_disable_timing` command is used to remove the effect of previous  `set_disable_timing` commands.

### Options

`-from`
: From pin of the disabled timing arc on an instance or cell.

`-to`
: To pin of the disabled timing arc on an instance or cell.

## unset_input_delay

```
unset_input_delay [-rise] [-fall] [-max] [-min] [-clock clock] [-clock_fall] port_pin_list
```

The `unset_input_delay` command removes a previously defined `set_input_delay`.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

`-clock`
: Unset the arrival time from clock.

`-clock_fall`
: Unset the arrival time from the falling edge of clock

## unset_output_delay

```
unset_output_delay [-rise] [-fall] [-max] [-min] [-clock clock] [-clock_fall] port_pin_list
```

The `unset_output_delay` command a previously defined `set_output_delay`.

### Options

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-max`
: Apply to maximum (setup) analysis.

`-min`
: Apply to minimum (hold) analysis.

`-clock`
: The arrival time is from this clock.

`-clock_fall`
: The arrival time is from the falling edge of clock

## unset_path_exceptions

```
unset_path_exceptions [-setup] [-hold] [-rise] [-fall] [-from from_list] [-rise_from from_list] [-fall_from from_list] [-through through_list] [-rise_through through_list] [-fall_through through_list] [-to to_list] [-rise_to to_list] [-fall_to to_list]
```

The `unset_path_exceptions` command removes any matching `set_false_path`, `set_multicycle_path`, `set_max_delay`, `set_min_delay`, and `set_path_margin` exceptions.

### Options

`-setup`
: Apply to setup checks.

`-hold`
: Apply to hold checks.

`-rise`
: Restrict the command to rising transitions.

`-fall`
: Restrict the command to falling transitions.

`-from`
: `from`: A list of clocks, instances, ports or pins.

`-rise_from`
: Restrict `-from` to rising transitions.

`-fall_from`
: Restrict `-from` to falling transitions.

`-through`
: `through`: A list of instances, pins or nets.

`-rise_through`
: Restrict `-through` to rising transitions.

`-fall_through`
: Restrict `-through` to falling transitions.

`-to`
: `to`: A list of clocks, instances, ports or pins.

`-rise_to`
: Restrict `-to` to rising transitions.

`-fall_to`
: Restrict `-to` to falling transitions.

## unset_power_activity

```
unset_power_activity [-global] [-input] [-input_ports ports] [-pins pins] [-clock clock]
```

The unset_power_activity_command is used to undo the effects of the `set_power_activity` command.

### Options

`-global`
: Unset the activity/duty for all non-clock pins.

`-input`
: Unset the default input port activity/duty.

`-input_ports`
: `input_ports`: Unset the input port activity/duty.

`-pins`
: `pins`: Unset the pin activity/duty.

`-clock`
: `clock`: Unset activity associated with this clock.

## unset_propagated_clock

```
unset_propagated_clock objects
```

Remove a previous `set_propagated_clock` command.

## unset_timing_derate

```
unset_timing_derate
```

Remove all derating factors set with the `set_timing_derate` command.

## unsuppress_msg

```
unsuppress_msg msg_ids
```

The `unsuppress_msg` command removes suppressions for the specified error/warning messages by ID. The list of message IDs can be found in doc/messages.md.

## user_run_time

```
user_run_time
```

Returns the total user cpu run time in seconds as a float.

## with_output_to_variable

```
with_output_to_variable var { cmds }
```

The `with_output_to_variable` command redirects the output of Tcl commands to a variable.

## write_path_spice

```
write_path_spice -path_args path_args -spice_file spice_file -lib_subckt_file lib_subckts_file -model_file model_file -power power -ground ground [-simulator hspice|ngspice|xyce]
```

The `write_path_spice` command writes a spice netlist for timing paths. Use path_args to specify `-from`/`-through`/`-to` as arguments to the `find_timing_paths` command. For each path, a spice netlist and the subckts referenced by the path are written in spice_directory. The spice netlist is written in path_<id>.sp and subckt file is path_<id>.subckt.

The spice netlists used by the path are written to subckt_file, which spice_file .includes. The device models used by the spice subckt netlists in model_file are also .included in spice_file. Power and ground names are specified with the `-power` and `-ground` arguments. The spice netlist includes a piecewise linear voltage source at the input and .measure statement for each gate delay and pin slew.

Example command:

```
write_path_spice -path_args {-from "in0" -to "out1" -unconstrained}  -spice_directory $result_dir  -lib_subckt_file "write_spice1.subckt"  -model_file "write_spice1.models"  -power VDD -ground VSS
```

When the simulator is hspice, .measure statements will be added to the spice netlist.

When the simulator is Xyce, the .print statement selects the CSV format and writes the waveform data to a file name path_<id>.csv so the results can be used by gnuplot.

### Options

`-path_args`
: `-from`|`-through`|`-to` arguments as in `report_checks`.

`-spice_file`
: Directory and path prefix for spice output files.

`-lib_subckt_file`
: Cell transistor level subckts.

`-model_file`
: Transistor model definitions .included by spice_file.

`-power`
: Voltage supply name in voltage_map of the default liberty library.

`-ground`
: Ground supply name in voltage_map of the default liberty library.

`-simulator`
: Simulator that will read the spice netlist.

## write_sdc

```
write_sdc [-mode mode] [-map_hpins] [-digits digits] [-gzip] [-no_timestamp] filename
```

Write the constraints for the design in SDC format to filename.

### Options

`-mode`
: SDC mode to write. The default is the current mode.

`-map_hpins`
: Map hierarchical pins to leaf pins in the SDC.

`-digits`
: Number of digits to print after the decimal point.

`-gzip`
: Compress the SDC with gzip.

`-no_timestamp`
: Do not include a time and date in the SDC file.

## write_sdf

```
write_sdf [-scene scene] [-divider /|.] [-include_typ] [-digits digits] [-gzip] [-no_timestamp] [-no_version] filename
```

Write the delay calculation delays for the design in SDF format to `filename`. If `-scene` is not specified the min/max delays are across all scenes. With `-scene` the min/max delays for that scene are written. The SDF TIMESCALE is the same as the time_unit in the first Liberty file read.

### Options

`-scene`
: Write delays for scene.

`-divider`
: Divider to use between hierarchy levels in pin and instance names.

`-include_typ`
: Include a 'typ' value in the SDF triple that is the average of min and max delays to satisfy some Verilog simulators that require three values in the delay triples.

`-digits`
: Number of digits to print after the decimal point.

`-gzip`
: Compress the SDF using gzip.

`-no_timestamp`
: Do not write a DATE statement.

`-no_version`
: Do not write a VERSION statement.

## write_timing_model

```
write_timing_model [-scene scene]  [-library_name lib_name] [-cell_name cell_name] filename
```

The `write_timing_model` command constructs a liberty timing model for the current design and writes it to filename. cell_name defaults to the cell name of the top level block in the design.

The SDC used to extract the block should include the clock definitions. If the block contains a clock network `set_propagated_clock` should be used so the clock delays are included in the timing model. The following SDC commands are ignored when building the timing model.

```
set_input_delay
set_output_delay
set_load
set_timing_derate
```

Using `set_input_transition` with the slew from the block context will be used will improve the match between the timing model and the block netlist.  Paths defined on clocks that are defined on internal pins are ignored because the model has no way to include the clock definition.

The resulting timing model can be used in a hierarchical timing flow as a replacement for the block to speed up timing analysis. This hierarchical timing methodology does not handle timing exceptions that originate or terminate inside the block. The timing model includes:

```
combinational paths between inputs and outputs
setup and hold timing constraints on inputs
clock to output timing paths
```

Resistance of long wires on inputs and outputs of the block cannot be modeled in Liberty. To reduce inaccuracies from wire resistance in technologies with resistive wires place buffers on inputs and ouputs.

The extracted timing model setup/hold checks are scalar (no input slew dependence). Delay timing arcs are load dependent but do not include input slew dependency.

### Options

`-scene`
: The scene to use for extracting the model.

`-library_name`
: The name to use for the liberty library. Defaults to cell_name.

`-cell_name`
: The name to use for the liberty cell. Defaults to the top level module name.

## write_verilog

```
write_verilog [-include_pwr_gnd] [-remove_cells cells] filename
```

The `write_verilog` command writes a Verilog netlist to filename. Use `-sort` to sort the instances so the results are reproducible across operating systems. Use `-remove_cells` to remove instances of lib_cells from the netlist.

### Options

`-include_pwr_gnd`
: Include power and ground pins on instances.

`-remove_cells`
: `lib_cells`: Liberty cells to remove from the Verilog netlist. Use `get_lib_cells`, a list of cells names, or a cell name with wildcards.

