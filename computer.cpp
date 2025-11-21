#include <iostream>
#include <vector>
#include <cstring>

#include "cpu.hpp"
#include "computer.hpp"

using namespace std;

Computer::Computer(int memory_size, int regs, int bus) : cpu(reg_num, bus_num) {
    reg_num = regs;
    bus_num = bus;
    memory.resize(memory_size, 0);
}

void Computer::load_program(const vector<RawInstruction> &prog_raw, int start_address) {
    vector<Instruction> prog = decode_program(prog_raw);
    for (size_t i = 0; i < prog.size(); i++){
        Instruction line = prog[i];
        memcpy(&memory[start_address + i], &line, sizeof(Instruction));
    }
}