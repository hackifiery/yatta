#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <cstring>

#include "cpu.hpp"
#include "assembler.hpp"
#include "computer.hpp"

using namespace std;

Computer::Computer(int memory_size, int regs, int bus) : cpu(regs, bus) {
    // initialize member counts
    reg_num = regs;
    bus_num = bus;
    // memory holds raw bytes now; memory_size is total bytes
    memory.resize(static_cast<size_t>(memory_size), 0); 
}

void Computer::put_program(const vector<RawInstruction> &prog_raw, int start_address) {
    if (start_address < 0) {
        throw runtime_error("start_address must be >= 0");
    }

    // decode_program now returns vector<Instruction>
    vector<Instruction> prog = decode_program(prog_raw);
    size_t prog_count = prog.size();
    if (prog_count == 0) return;

    const size_t instr_size = sizeof(Instruction);
    size_t required_bytes = (static_cast<size_t>(start_address) + prog_count) * instr_size;
    if (required_bytes > memory.size()) {
        throw runtime_error("Not enough memory to load program at given start_address (bytes)");
    }

    // memcpy each Instruction into the raw byte memory at the correct byte offset
    uint8_t* mem_ptr = memory.data();
    for (size_t i = 0; i < prog_count; ++i) {
        memcpy(mem_ptr + (static_cast<size_t>(start_address) + i) * instr_size,
               &prog[i],
               instr_size);
    }
}

void Computer::put_program(const vector<Instruction> &prog, int start_address) {
    if (start_address < 0) {
        throw runtime_error("start_address must be >= 0");
    }

    size_t prog_count = prog.size();
    if (prog_count == 0) return;

    const size_t instr_size = sizeof(Instruction);
    size_t required_bytes = (static_cast<size_t>(start_address) + prog_count) * instr_size;
    if (required_bytes > memory.size()) {
        throw runtime_error("Not enough memory to load program at given start_address (bytes)");
    }

    uint8_t* mem_ptr = memory.data();
    for (size_t i = 0; i < prog_count; ++i) {
        memcpy(mem_ptr + (static_cast<size_t>(start_address) + i) * instr_size,
               &prog[i],
               instr_size);
    }
}

Instruction Computer::read_program(int start_address) {
    const size_t instr_size = sizeof(Instruction);
    if (start_address < 0) throw runtime_error("read_program: start_address must be >= 0");
    size_t byte_offset = static_cast<size_t>(start_address) * instr_size;
    if (byte_offset + instr_size > memory.size()) {
        throw runtime_error("read_program: start_address out of range (bytes)");
    }
    Instruction inst;
    memcpy(&inst, memory.data() + byte_offset, instr_size);
    return inst;
}

void Computer::run_from_ram(int start_address) {
    const size_t instr_size = sizeof(Instruction);
    if (start_address < 0) throw runtime_error("run_from_ram: start_address must be >= 0");
    // Check that start PC maps to valid instruction memory
    size_t byte_offset_check = static_cast<size_t>(start_address) * instr_size;
    if (byte_offset_check >= memory.size()) throw runtime_error("run_from_ram: start_address out of range");

    cpu.pc = start_address;
    cpu.increment_pc = true;

    while (cpu.pc >= 0 && static_cast<size_t>(cpu.pc) * instr_size < memory.size()) {
        if (cpu.halted) break;
        // read instruction bytes into local Instruction
        Instruction inst;
        memcpy(&inst, memory.data() + static_cast<size_t>(cpu.pc) * instr_size, instr_size);

        // If the instruction has a condition, check it using cpu.check_condition
        bool execute = true;
        if (inst.comp[0] != '\0') {
            if (!cpu.check_condition(inst)) {
                execute = false;
            }
        }

        if (execute) {
            cpu.exec_line(inst);
        }

        // PC increment logic mirrors Cpu::exec_prog behavior
        if (cpu.increment_pc) {
            cpu.pc++;
        } else {
            cpu.increment_pc = true; // reset flag after a successful jump
        }
    }
}
