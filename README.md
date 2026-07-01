# CS422 PIN Profiler

A collection of Intel PIN-based dynamic instrumentation assignments done under CS422 (Computer Architecture) at IITK.  
This repository includes tools for instruction profiling, memory footprint analysis, fast-forward execution, and side-channel-style trace extraction for RSA/AES-related exercises.

## Overview

This project is organized around three homework modules:

- **HW1**: Instruction and memory profiling with a PIN tool
- **HW2**: RSA trace analysis and key extraction from multiplication timing patterns
- **HW3**: Memory trace generation and analysis for AES-related workflows

The codebase is primarily written in:

- **C++** for the PIN tool instrumentation logic
- **C** for target programs and analysis harnesses
- **Python** for trace post-processing and validation
- **Shell** for runner scripts
- **Makefile** for build integration with Intel PIN

## Repository Structure

```text
.
├── hw2/
│   ├── extract_key.py
│   └── readme.txt
├── hw3/
│   ├── analysis.c
│   └── check.py
├── scripts/
│   └── run.sh
└── tool/
    └── HW1/
        ├── HW1.cpp
        ├── dummy.c
        └── makefile
```

## Features

### HW1: PIN Instruction Profiler
The HW1 tool instruments a target binary and records:

- instruction mix / opcode-class statistics
- CPI estimation
- load/store counts
- control-flow counts
- memory footprint over 32-byte chunks
- instruction-length and operand-count distributions
- immediate/displacement value ranges
- runtime with optional fast-forwarding

The tool supports:
- **fast-forwarding** by a configurable number of billions of instructions
- **output file selection**
- **per-instruction and memory-access accounting**

### HW2: RSA Trace Key Extraction
The HW2 folder focuses on recovering a secret key from execution traces by analyzing repeated instruction-pointer patterns. The workflow:

1. collect a trace from a PIN-instrumented RSA binary
2. isolate the most frequent instruction pointer
3. measure gaps between repeated executions
4. infer bit patterns from short/long timing gaps
5. write the recovered key to `key.txt`

### HW3: Memory Trace Validation
HW3 contains a harness that:

- generates plaintext inputs
- compiles a small AES-based program
- runs it under PIN
- checks the resulting trace output
- validates whether the expected behavior passes or fails

## Prerequisites

To build and run the PIN tool, you will need:

- **Intel PIN** installed and available via `PIN_ROOT`
- a Linux environment
- `g++` / `gcc`
- `make`
- `python3`
- OpenSSL development libraries for HW3 (`libcrypto`)

Example package requirements on Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential python3 python3-pip libssl-dev
```

## Setup

### 1. Set `PIN_ROOT`
Make sure the Intel PIN kit is installed and the environment variable is set:

```bash
export PIN_ROOT=/path/to/pin
```

### 2. Build the HW1 tool
From the `tool/HW1` directory:

```bash
make
```

This should produce the PIN shared library for the homework tool.

### 3. Prepare the runner script
The script at `scripts/run.sh` launches PIN with:

- the PIN executable
- the compiled tool `.so`
- a target binary
- an output file name
- optional arguments to pass through to the target

You may need to update the project root paths in the script to match your local directory layout.

## Usage

### HW1: Run the profiler
The runner script usage is:

```bash
./scripts/run.sh <binary_path> <output_name> <fast_forward_billions> [args...]
```

Example:

```bash
./scripts/run.sh tool/HW1/dummy32 dummy_test 0
```

This will:

- run the selected binary under PIN
- record profiler statistics
- save results in the configured results directory

### HW2: Extract RSA key from a trace
Run the Python script against a trace file:

```bash
python3 hw2/extract_key.py
```

By default, the script expects:

- `mul_trace.txt` as input
- `key.txt` as output

The script identifies the dominant instruction pointer in the trace and reconstructs a likely key from multiplication timing gaps.

### HW3: Generate and verify AES trace behavior
The HW3 harness can be used to compile and run the sample analysis program:

```bash
python3 hw3/check.py
```

This script:

- creates a plaintext input file
- builds the C analysis program
- executes it under PIN
- checks the resulting output

## HW1 Output Format

The HW1 tool writes a structured report that includes:

- instruction count
- CPI
- instruction class breakdown
- memory footprint statistics
- operand and length histograms
- immediate and displacement extremes
- runtime summary

Example sections include:

- `PART A/B`: instruction profile and CPI
- `PART C`: memory footprint
- `PART D`: IA-32 ISA properties

## Scripts

### `scripts/run.sh`
A convenience wrapper for launching PIN instrumentation runs.

Key variables inside the script:

- `PROJECT_ROOT`
- `PIN_EXE`
- `TOOL_SO`
- `RESULTS_DIR`

If your repository is located somewhere else, update `PROJECT_ROOT` accordingly.

## Notes

- Some paths in scripts are hardcoded and should be adjusted for your environment.
- The HW1 tool is tuned for a specific assignment layout and output format.
- HW2 and HW3 scripts assume their supporting binaries and trace files are present in the working directory.

## Example Workflow

### Build and run HW1
```bash
cd tool/HW1
make
cd ../..
./scripts/run.sh tool/HW1/dummy32 hw1_demo 0
```

### Run HW2 trace extraction
```bash
cd hw2
python3 extract_key.py
```

### Run HW3 validation
```bash
cd hw3
python3 check.py
```
