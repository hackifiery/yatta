#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdint>

#include "cpu.hpp"
#include "computer.hpp"

using namespace std;

Computer::Computer(int memory_size, int regs, int bus) : cpu(regs, bus) {
    // initialize member counts
    reg_num = regs;
    bus_num = bus;
    // memory holds Instruction slots now
    memory.resize(static_cast<size_t>(memory_size)); 
}

void Computer::put_program(const vector<RawInstruction> &prog_raw, int start_address) {
    if (start_address < 0) {
        throw runtime_error("start_address must be >= 0");
    }

    vector<Instruction> prog = decode_program(prog_raw);
    size_t prog_count = prog.size();
    if (prog_count == 0) return;

    // bounds-check using instruction slots
    if (static_cast<size_t>(start_address) + prog_count > memory.size()) {
        throw runtime_error("Not enough memory (instruction slots) to load program at given start_address");
    }

    // Copy Instructions into the instruction memory safely via assignment
    for (size_t i = 0; i < prog_count; ++i) {
        memory[static_cast<size_t>(start_address) + i] = prog[i];
    }
}

Instruction Computer::read_program(int start_address) {
    // only reads one line
    if (start_address < 0 || static_cast<size_t>(start_address) >= memory.size()) {
        throw runtime_error("read_program: start_address out of range");
    }
    // Return the instruction without executing it (execution should be driven by run_from_ram)
    Instruction inst = memory[static_cast<size_t>(start_address)];
    return inst;
}

void Computer::run_from_ram(int start_address) {
    if (start_address < 0 || static_cast<size_t>(start_address) >= memory.size()) {
        throw runtime_error("run_from_ram: start_address out of range");
    }

    // Initialize CPU PC to the start address and drive execution using cpu.pc
    cpu.pc = start_address;
    cpu.increment_pc = true;

    while (!cpu.halted && cpu.pc >= 0 && static_cast<size_t>(cpu.pc) < memory.size()) {
        Instruction& inst = memory[static_cast<size_t>(cpu.pc)];
        cpu.exec_line(inst);

        // PC increment logic mirrors Cpu::exec_prog behavior
        if (cpu.increment_pc) {
            cpu.pc++;
        } else {
            cpu.increment_pc = true; // reset flag after a successful jump
        }
    }
}
