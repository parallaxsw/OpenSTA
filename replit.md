# OpenSTA - Parallax Static Timing Analyzer

## Overview
OpenSTA is a gate-level static timing verifier used to analyze and verify the timing of digital circuit designs. This is a command-line tool that uses a TCL interpreter to read designs, specify timing constraints, and generate timing reports.

**Version:** 2.7.0  
**License:** GPL v3  
**Language:** C++  
**Build System:** CMake

## Project Status
The project has been successfully built and configured to run in the Replit environment. The main executable `sta` is located in the `build/` directory.

## Recent Changes
- **2025-11-20:** Implemented dual-edge power activity propagation mode
  - Added `activity_propagation_dual_edge_` flag to Power class
  - Modified `evalBddActivity()` to multiply density by 2 in dual-edge mode
  - Added TCL commands: `set_power_activity_dual_edge` and `report_power_activity_dual_edge`
  - Enables pessimistic power estimation for combinational circuits by accounting for both clock edges
- **2024-11-20:** Initial setup in Replit environment
  - Installed all required dependencies (CMake, TCL, SWIG, Bison, Flex, Eigen, CUDD, Zlib)
  - Built CUDD library from source (required dependency)
  - Configured and built OpenSTA with CMake
  - Fixed minor compilation issue in `include/sta/Zlib.hh` (added missing `gzgetc` macro for systems without zlib)
  - Created workflow to run OpenSTA interactively

## Project Architecture

### Key Components
1. **Timing Analysis Engine** (`search/`) - Core static timing analysis algorithms
2. **Liberty Library Support** (`liberty/`) - Liberty file parser and representation
3. **Delay Calculation** (`dcalc/`) - Various delay calculation algorithms including Arnoldi, DMP, Prima
4. **Network Representation** (`network/`) - Abstract circuit network representation
5. **Parasitic Support** (`parasitics/`) - SPEF parasitic file reader and handling
6. **Power Analysis** (`power/`) - VCD and SAIF power activity analysis
7. **SDC Constraints** (`sdc/`) - Synopsys Design Constraints reader and writer
8. **Verilog Support** (`verilog/`) - Verilog netlist reader and writer

### Supported File Formats
- **Verilog netlist** (`.v`)
- **Liberty library** (`.lib`)
- **SDC timing constraints** (`.sdc`)
- **SDF delay annotation** (`.sdf`)
- **SPEF parasitics** (`.spef`)
- **VCD power activities** (`.vcd`)
- **SAIF power activities** (`.saif`)

### Build Dependencies
- **CMake 3.10+** - Build system
- **C++ Compiler** (GCC 11.4+ or Clang 15+)
- **TCL 8.6** - Command interpreter
- **SWIG 3.0+** - TCL bindings generator
- **Bison 3.2+** - Parser generator
- **Flex 2.6+** - Lexer generator
- **Eigen 3.4** - Matrix library (required)
- **CUDD 3.0** - Binary Decision Diagram package (required)
- **Zlib 1.2+** - Compression library (optional)

## How to Use

### Running OpenSTA
The workflow "OpenSTA" is configured to run the interactive TCL shell. You can also run it manually:

```bash
./build/sta
```

### Command-line Options
- `-help` - Show help and exit
- `-version` - Show version and exit
- `-no_init` - Do not read .sta init file
- `-threads count|max` - Use multiple threads
- `-no_splash` - Do not show license splash
- `-exit` - Exit after reading cmd_file
- `cmd_file` - Source a TCL command file

### Example Usage
There are example files in the `examples/` directory. To run an example:

```tcl
# In the OpenSTA TCL shell
source examples/delay_calc.tcl
```

Or from command line:
```bash
./build/sta examples/delay_calc.tcl
```

### Basic TCL Commands
OpenSTA provides TCL commands for timing analysis:
- `read_liberty` - Read Liberty library file
- `read_verilog` - Read Verilog netlist
- `link_design` - Link the design
- `read_sdc` - Read SDC constraints
- `read_spef` - Read SPEF parasitics
- `report_checks` - Report timing paths
- `report_worst_slack` - Report worst slack
- `report_power` - Report power consumption
- `set_power_activity` - Set power activity for pins/ports
- `set_power_activity_dual_edge` - Enable/disable dual-edge power propagation mode
- `report_power_activity_dual_edge` - Report current dual-edge mode status
- See `doc/OpenSTA.pdf` for complete command reference

### Dual-Edge Power Activity Propagation Mode

This custom feature enables more pessimistic power estimation for combinational circuits by assuming signals can switch on both the rising and falling edges of the clock.

**Background:**
- Standard single-edge mode: Signals toggle once per clock cycle (on one edge)
- Dual-edge mode: Signals can toggle on both posedge and negedge, effectively allowing 2 transitions per cycle
- This results in 2x higher switching activity density for combinational logic

**Mathematical Impact:**
For a NAND2 gate with input activities `density(A) = density(B) = 0.5` and `duty(A) = 0.5`:
- Single-edge mode: `density(Z) = 0.375`
- Dual-edge mode: `density(Z) = 0.75` (2x multiplier applied)

**TCL Commands:**

Enable dual-edge mode:
```tcl
set_power_activity_dual_edge on
```

Disable dual-edge mode (default):
```tcl
set_power_activity_dual_edge off
```

Check current mode status:
```tcl
report_power_activity_dual_edge
```

**Use Cases:**
- Conservative worst-case power analysis
- Designs with both posedge and negedge triggered logic
- Pessimistic power budgeting for combinational circuits

**Implementation Details:**
- Modified `Power::evalBddActivity()` in `power/Power.cc` to apply 2x multiplier when dual-edge mode is enabled
- Activity propagation is invalidated when mode changes to ensure fresh calculations
- Mode is disabled by default to maintain backward compatibility

## Directory Structure
```
.
├── app/          - Main application entry point
├── build/        - Build output directory (contains 'sta' executable)
├── cmake/        - CMake helper modules
├── dcalc/        - Delay calculation algorithms
├── doc/          - Documentation (PDF, API docs, changelog)
├── examples/     - Example designs and scripts
├── graph/        - Timing graph implementation
├── include/      - Public header files
├── liberty/      - Liberty file parser
├── network/      - Network representation
├── parasitics/   - Parasitic extraction support
├── power/        - Power analysis
├── sdc/          - SDC constraint handling
├── sdf/          - SDF file support
├── search/       - Static timing analysis engine
├── spice/        - SPICE output support
├── tcl/          - TCL integration
├── test/         - Test files and regression tests
├── util/         - Utility functions
└── verilog/      - Verilog parser
```

## Development Notes

### Rebuilding
If you modify the source code:
```bash
cd build
make -j4
```

### Clean Build
To perform a clean build:
```bash
rm -rf build/*
cd build
cmake -DCUDD_DIR=/tmp/cudd-install \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DTCL_LIBRARY=/nix/store/bqppwwi9g8nzbk0b6hq6fwkqnwd06y63-tcl-8.6.15/lib/libtcl8.6.so \
  -DTCL_HEADER=/nix/store/bqppwwi9g8nzbk0b6hq6fwkqnwd06y63-tcl-8.6.15/include/tcl.h \
  -DEigen3_DIR=/nix/store/gbqqdl9r1xvcydlaaybhr4mfk8qkqivn-eigen-3.4.0-unstable-2022-05-19/share/eigen3/cmake \
  -DZLIB_ROOT=/nix/store/0w4q8asq9sn56dl0sxp1m8gk4vy2ygs8-zlib-1.3.1-dev \
  -DFLEX_INCLUDE_DIR=/nix/store/cl3plks24gm9myzfa6j6zg7575jcmhfr-flex-2.6.4/include \
  ..
make -j4
```

### Code Modifications
- **Fixed:** Added `gzgetc` macro to `include/sta/Zlib.hh` for compatibility when zlib is not fully linked

## Resources
- **Documentation:** See `doc/OpenSTA.pdf` for complete command reference
- **API Reference:** See `doc/StaApi.txt` for timing engine API
- **Changelog:** See `doc/ChangeLog.txt` for version history
- **GitHub:** https://github.com/parallaxsw/OpenSTA
- **License:** `doc/CLA.txt` for contributor agreement

## Notes
- This is a professional timing analysis tool used in EDA (Electronic Design Automation)
- The tool is designed to be integrated into larger EDA flows
- OpenSTA can be used standalone or as a timing engine for other tools
- The project is maintained exclusively by Parallax Software
