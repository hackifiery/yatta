#pragma once

#include "cpu.hpp"
#include <vector>

class Computer {
public:
    int reg_num, bus_num;
    Computer(int memory_size, int reg, int bus);
    std::vector<Instruction> memory; // store Instructions directly (safe)
    Cpu cpu;
    void put_program(const std::vector<RawInstruction>& prog_raw, int start_address);
    Instruction read_program(int start_address);
    void run_from_ram(int start_address);
};