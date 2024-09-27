# MIPS Pipeline Simulator

This project is a **C++** simulator that models the behavior of a **MIPS pipeline processor**. The simulator demonstrates the execution of a limited set of MIPS instructions in a pipeline, providing insights into the execution of basic instructions and pipeline hazards.

## Supported Instructions

The simulator supports the following **MIPS instructions**:
- **add**: Add two registers.
- **sub**: Subtract two registers.
- **lw**: Load word from memory to register.
- **sw**: Store word from register to memory.
- **beq**: Branch if equal.

## Features

- **Instruction Fetch (IF)**: Fetches instructions from memory.
- **Instruction Decode (ID)**: Decodes the instructions and prepares operands.
- **Execution (EX)**: Executes arithmetic and logical operations.
- **Memory Access (MEM)**: Reads and writes from/to memory.
- **Write Back (WB)**: Writes the result back to the register file.
- **Pipeline Hazards Handling**: Manages data, control, and structural hazards.

## How It Works

The simulator mimics the 5-stage MIPS pipeline:

1. **Instruction Fetch (IF)**: Fetches the instruction to be executed from instruction memory.
2. **Instruction Decode (ID)**: Decodes the instruction and determines register usage or memory access.
3. **Execute (EX)**: Performs the arithmetic/logical operation or computes memory addresses for load/store.
4. **Memory Access (MEM)**: Accesses memory for `lw` and `sw` instructions.
5. **Write Back (WB)**: Writes the computed result back to the register file (if applicable).

## Pipeline Hazards

The simulator handles common pipeline hazards:
- **Data Hazards**: Managed by forwarding or stalling (introducing bubbles).
- **Control Hazards**: Handled using a simple mechanism (such as stalling for `beq`).
- **Structural Hazards**: Handled by detecting and managing overcommitment of pipeline resources.

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/yourusername/MIPS-pipeline-simulator.git
