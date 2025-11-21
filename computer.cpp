#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdint>

#include "cpu.hpp"
#include "computer.hpp"

using namespace std;

Computer::Computer(int memory_size, int regs, int bus) : cpu(regs, bus) {
    // initialize member counts
    reg_num = regs;
    bus_num = bus;
    memory.resize(memory_size, 0);
}

void Computer::put_program(const vector<RawInstruction> &prog_raw, int start_address) {
    if (start_address < 0) {
        throw runtime_error("start_address must be >= 0");
    }

    vector<Instruction> prog = decode_program(prog_raw);
    size_t prog_count = prog.size();
    if (prog_count == 0) return;

    // Calculate sizes in bytes
    size_t instr_size = sizeof(Instruction);
    size_t mem_elem_size = sizeof(memory[0]);
    size_t mem_bytes = memory.size() * mem_elem_size;
    size_t required_bytes = static_cast<size_t>(start_address + prog_count) * instr_size;

    if (required_bytes > mem_bytes) {
        throw runtime_error("Not enough memory to load program at given start_address");
    }

    // Copy each Instruction into the memory buffer at byte offsets.
    // Use byte-level pointer to memory.data() so this works regardless of underlying element type.
    char* mem_bytes_ptr = reinterpret_cast<char*>(memory.data());
    for (size_t i = 0; i < prog_count; ++i) {
        memcpy(mem_bytes_ptr + (static_cast<size_t>(start_address) + i) * instr_size,
               &prog[i],
               instr_size);
    }
}

Instruction Computer::read_program(int start_address) {
    // only reads one line so we dont have to load the entire program into a buffer before executing it (we can just do so directly via the exec_line func)
    const size_t instr_size = sizeof(Instruction);
    uint8_t* mem_bytes_ptr = reinterpret_cast<uint8_t*>(memory.data());
    Instruction inst;
    memcpy(&inst, mem_bytes_ptr + static_cast<size_t>(start_address) * instr_size, instr_size);
    cpu.exec_line(inst); // execute the instruction immediately after reading
    return inst;
}
