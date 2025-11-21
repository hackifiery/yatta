#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <limits>
#include <map>
#include <string>

using namespace std;

// --- Configuration ---
constexpr int NUM_REGISTERS_SAMPLE = 8;
constexpr int BUS_COUNT_SAMPLE = 1;

// Declare globals as extern here; definitions live in cpu.cpp
extern string ops_ordered[6];
extern map<string, string> ops_map;

struct Instruction {
    int source_type;
    int source_value;
    int dest_type;
    int dest_value;
    // Condition fields
    string cond1; // Left-hand side (usually a flag name like "ZF")
    string cond2; // Right-hand side (usually a constant like "1")
    string comp;  // Comparison type (e.g., "eq" for ==)
};

struct RawInstruction {
    string src;
    string dest;
    string condition;
};

string trim(const string& str);
Instruction convert_line(const RawInstruction& line_raw);
vector<Instruction> decode_program(const vector<RawInstruction>& prog_raw);

class Cpu {
private:
    bool check_add_overflow(int a, int b, int& result);
    bool check_mul_overflow(int a, int b, int& result);
    int update_alu();
    int get_flag_value(const string& flag_name) const;
    bool check_condition(const Instruction& instr);

public:
    bool increment_pc = true;
    int pc = 0;
    
    vector<unsigned int> regs, bus;
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

    int exec_prog(const vector<Instruction>& prog); // DEBUG
    void print_register_file();

    int exec_line(const Instruction& instr);
};

int run_prog(const vector<RawInstruction>& prog_raw);// DEBUG

extern vector<RawInstruction> sample_program;

void test();
