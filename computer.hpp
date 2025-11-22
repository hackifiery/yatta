#pragma once

#include "cpu.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

class Computer {
public:
    int reg_num, bus_num;
    Computer(int memory_size, int reg, int bus);
    std::vector<uint8_t> memory; // now a raw byte buffer
    Cpu cpu;
    void put_program(const std::vector<RawInstruction>& prog_raw, int start_address);
    Instruction read_program(int start_address);
    void run_from_ram(int start_address);
};