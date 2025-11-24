# OpenSTA - Static Timing Analyzer

## Overview

OpenSTA is a gate-level static timing verification tool used to analyze and verify the timing of digital circuit designs. It operates as a standalone executable that processes standard Electronic Design Automation (EDA) file formats including Verilog netlists, Liberty libraries, SDC timing constraints, SDF delay annotations, SPEF parasitics, and power activity files (VCD/SAIF). The tool uses a TCL command interpreter for design input, constraint specification, and timing report generation.

The architecture is designed to function both as a standalone timing analyzer and as an embeddable timing engine that can be integrated into other EDA tools through a network adapter interface, allowing host applications to leverage timing analysis capabilities without duplicating netlist data structures.

## User Preferences

Preferred communication style: Simple, everyday language.

## System Architecture

### Core Language and Build System

- **Language**: C++17 with heavy use of Standard Template Library (STL)
- **Build System**: CMake (minimum version 3.10)
- **Build Location**: Artifacts generated in `build/` directory
- **Primary Outputs**: 
  - `libOpenSTA` - Core library for embedding
  - `sta` - Standalone executable

### Modular Component Architecture

The codebase is organized into focused subsystems, each handling a specific aspect of timing analysis:

**Utilities and Foundation** (`util/`)
- Basic utility classes and data structures used across the application

**Library Management** (`liberty/`)
- Liberty timing library parsing and representation
- Handles cell timing characteristics and power models
- Uses Bison/Flex for parsing Liberty format files

**Network Abstraction** (`network/`)
- Defines network and library API used throughout the codebase
- Provides abstraction layer allowing different netlist representations
- Enables the tool to work with host application data structures via adapters

**Verilog Processing** (`verilog/`)
- Verilog netlist reader implementing the network API
- Uses Bison/Flex for parsing Verilog files

**Timing Graph** (`graph/`)
- Core timing graph data structure built from network and cell timing arcs
- Represents timing paths through the design

**Constraint Management** (`sdc/`)
- SDC (Synopsys Design Constraints) timing constraint classes
- Supports clocks (generated, gated, multiple frequencies), exception paths (false paths, multicycle, min/max delays), and latency specifications

**Delay Annotation** (`sdf/`)
- SDF (Standard Delay Format) reader, writer, and annotator
- Back-annotation of timing delays from layout tools

**Delay Calculation** (`dcalc/`)
- Delay calculator API with multiple implementations
- Integrated Dartu/Menezes/Pileggi RC effective capacitance algorithm (Arnoldi method)
- Supports external delay calculator plugins

**Timing Analysis Engine** (`search/`)
- Core search/traversal algorithms for timing analysis
- Computes arrival times, required times, and slack
- Incremental update support for efficient re-analysis

**Parasitic Handling** (`parasitics/`)
- SPEF (Standard Parasitic Exchange Format) and SPF parasitic file processing
- RC network representation for accurate delay modeling

**Power Analysis** (`power/`)
- Power activity propagation and estimation
- Supports VCD and SAIF activity file formats
- Dual-edge activity propagation mode for pessimistic power estimation

### Design Patterns and Principles

**Network Adapter Pattern**: The network API provides an abstraction layer allowing OpenSTA to work with different netlist representations without code duplication. Host applications implement adapters to expose their netlist data structures through the standard network interface.

**Incremental Analysis**: The timing engine supports query-based incremental updates, allowing efficient re-computation when design or constraints change rather than full re-analysis.

**Plugin Architecture**: Delay calculation supports external calculators through a defined API, enabling custom delay models.

**TCL Command Interface**: All user interaction occurs through TCL commands, providing both interactive and scriptable control. SWIG is used to generate TCL bindings from C++ classes.

### File Format Support

**Input Formats**:
- Verilog (netlist)
- Liberty (.lib - timing libraries)
- SDC (timing constraints)
- SDF (delay back-annotation)
- SPEF/SPF (parasitics)
- VCD/SAIF (power activity)

**Parser Implementation**: Uses Flex for lexical analysis and Bison for grammar parsing across multiple file formats (Liberty, Verilog, SDF, SPEF, SAIF).

### Code Organization Conventions

- Header files use `.hh` extension, implementation files use `.cc`
- Class names: UpperCamelCase
- Member functions: lowerCamelCase
- Member variables: snake_case with trailing underscore
- Directory-level class declaration headers named `DirectoryClass.hh` to minimize header dependencies
- Prefer pointer members over embedded instances to reduce compilation dependencies

## External Dependencies

### Required Libraries

**Zlib**: Compression library for reading compressed Liberty, Verilog, SDF, SPF, and SPEF files.

**TCL**: TCL interpreter integration for command-line interface and scripting.

**CUDD**: BDD (Binary Decision Diagram) package required for logic function representation and manipulation.

**SWIG**: Simplified Wrapper and Interface Generator for creating TCL bindings to C++ classes.

**Bison/Flex**: Parser and lexer generators for file format processing.

### Build Tool Dependencies

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 14.2+ in current build)
- Standard build tools (ar, ranlib, linker)

### Optional Features

**TCL Readline**: Optional TCL readline package support (enabled by default via `USE_TCL_READLINE` option).

**Sanitizers**: Optional compile-time support for Thread Sanitizer (`ENABLE_TSAN`) and Address Sanitizer (`ENABLE_ASAN`) for debugging.

### Operating Environment

The project is designed to run on Unix-like systems (Linux, macOS) and uses Nix for reproducible builds as evidenced by the build configuration. No database dependencies are present - all data is file-based or in-memory.