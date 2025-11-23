#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <limits>
#include <map>



using namespace std;

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

    // Comparison mnemonic (fixed-size so Instruction is trivially-copyable)
    char comp[8];  // e.g., "eq","ne","lt",...

    // Typed condition operands: (type, value) pairs, same format as source/dest
    // type: 0=const,1=reg,2=alu,3=flag,4=pc
    int cond1_type = -1;
    int cond1 = 0; // numeric operand (mirrors source_value)
    int cond2_type = -1;
    int cond2 = 0; // numeric operand (mirrors source_value)
};

struct RawInstruction {
    std::string src;
    std::string dest;
    std::string condition;
};

class Cpu {
private:
    bool check_add_overflow(int a, int b, int& result);
    bool check_mul_overflow(int a, int b, int& result);
    int update_alu();
    int get_flag_value(const char* flag_name) const;

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
    bool check_condition(const Instruction& instr);

    int exec_line(const Instruction& instr);
};

int run_prog(const std::vector<RawInstruction>& prog_raw);// DEBUG

extern std::vector<RawInstruction> sample_program;
