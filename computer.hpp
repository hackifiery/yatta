#pragma once

#include "cpu.hpp"
#include <vector>

class Computer {
public:
    int reg_num, bus_num;
    Computer(int memory_size, int reg, int bus);
    vector<int> memory;
    Cpu cpu;
    void load_program(const vector<RawInstruction>& prog_raw, int start_address);
};
