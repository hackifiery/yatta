#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <limits>
#include <map>

// --- Configuration ---
constexpr int NUM_REGISTERS_SAMPLE = 8;
constexpr int BUS_COUNT_SAMPLE = 1;

// Declare globals as extern here; definitions live in cpu.cpp
extern std::string ops_ordered[6];
extern std::map<std::string, std::string> ops_map;

struct Instruction {
    int source_type = 0;
    int source_value = 0;
    int dest_type = 0;
    int dest_value = 0;
    // Condition fields
    std::string cond1 = ""; // Left-hand side (usually a flag name like "ZF")
    std::string cond2 = ""; // Right-hand side (usually a constant like "1")
    std::string comp  = ""; // Comparison type (e.g., "eq" for ==)
};

struct RawInstruction {
    std::string src;
    std::string dest;
    std::string condition;
};

std::string trim(const std::string& str);
Instruction convert_line(const RawInstruction& line_raw);
std::vector<Instruction> decode_program(const std::vector<RawInstruction>& prog_raw);

class Cpu {
private:
    bool check_add_overflow(int a, int b, int& result);
    bool check_mul_overflow(int a, int b, int& result);
    int update_alu();
    int get_flag_value(const std::string& flag_name) const;
    bool check_condition(const Instruction& instr);

public:
    int halted = 0;
    bool increment_pc = true;
    int pc = 0;
    
    std::vector<unsigned int> regs, bus;
    int alu[3] = { 0,0,0 };
    int* alu_result = &alu[0]; 
    int* alu_op1 = &alu[1]; 
    int* alu_op2 = &alu[2]; 
    int alu_regs[3] = { 0,0,0 };
    unsigned int alu_trigger = 0; 
    int* alu_zf = &alu_regs[0]; // zero flag
    int* alu_nf = &alu_regs[1]; // negative flag
    int* alu_of = &alu_regs[2]; // overflow flag

    int reg_amount = 0, bus_amount = 0;
    Cpu(int reg, int bus_);

    int exec_prog(const std::vector<Instruction>& prog); // DEBUG
    void print_register_file();

    int exec_line(const Instruction& instr);
};

int run_prog(const std::vector<RawInstruction>& prog_raw);// DEBUG

extern std::vector<RawInstruction> sample_program;

void test();
