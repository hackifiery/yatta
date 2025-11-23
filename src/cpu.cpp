#include <cstdint>
#include <cstring>

#include "cpu.hpp"
#include "assembler.hpp"
#include "parser.hpp"

using namespace std;


// Definitions moved from header
string ops_ordered[6] = {"==", "!=", "<=", ">=", "<", ">"};
map<string, string> ops_map = {
    {"==", "eq"},
    {"!=", "ne"},
    {"<", "lt"},
    {"<=", "le"},
    {">", "gt"},
    {">=", "ge"}
};

// Cpu method definitions

bool Cpu::check_add_overflow(int a, int b, int& result) {
    if (b > 0 && a > numeric_limits<int>::max() - b) {
        return true; // Overflow
    }
    if (b < 0 && a < numeric_limits<int>::min() - b) {
        return true; // Underflow
    }
    result = a + b;
    return false; // No overflow
}

bool Cpu::check_mul_overflow(int a, int b, int& result) {
    if (a == 0 || b == 0) {
        result = 0;
        return false;
    }
    if (a > 0 && b > 0 && a > numeric_limits<int>::max() / b) return true;
    if (a < 0 && b < 0 && a < numeric_limits<int>::max() / b) return true;
    if (a > 0 && b < 0 && b < numeric_limits<int>::min() / a) return true;
    if (a < 0 && b > 0 && a < numeric_limits<int>::min() / b) return true;
    
    result = a * b;
    return false; // No overflow
}

int Cpu::update_alu() {
    if (alu_trigger) {
        int result = 0;
        // Clear flags before operation
        *alu_of = 0; 
        *alu_nf = 0; 
        *alu_zf = 0; 

        switch (alu_trigger) {
        case 1: // ADD
            if (check_add_overflow(alu[1], alu[2], result)) *alu_of = 1;
            alu[0] = result;
            break;
        case 2: // SUB (A - B)
            if (check_add_overflow(alu[1], -alu[2], result)) *alu_of = 1;
            alu[0] = result;
            break;
        case 3: // MUL
            if (check_mul_overflow(alu[1], alu[2], result)) *alu_of = 1;
            alu[0] = result;
            break;
        case 4: // DIV
            if (alu[2] != 0) {
                alu[0] = alu[1] / alu[2];
            }
            else {
                throw runtime_error("Division by zero in ALU");
            }
            break;
        default:
            break;
        }

        // Set flags based on the final signed result
        if (alu[0] == 0) {
            *alu_zf = 1;
        }
        if (alu[0] < 0) {
            *alu_nf = 1;
        }

        alu_trigger = 0; // Reset trigger after operation
    }
    return 0;
}

int Cpu::get_flag_value(const char* flag_name) const {
    if (flag_name == nullptr) throw runtime_error("Null flag name");
    if (strcmp(flag_name, "ZF") == 0) return *alu_zf;
    if (strcmp(flag_name, "NF") == 0) return *alu_nf;
    if (strcmp(flag_name, "OF") == 0) return *alu_of;
    // allow HF/AF if needed by caller (AF handled in source_type==3)
    throw runtime_error(string("Unknown flag name in condition: ") + flag_name);
}

bool Cpu::check_condition(const Instruction& instr) {
    if (instr.comp[0] == '\0') {
        return true; // No condition, always execute
    }
    // Compute operand values from declared types and numeric condition values stored in Instruction.
    auto compute_value = [&](int type, int value)->int {
        if (type == -1) throw runtime_error("Missing condition operand type; machine-code must declare types");
        switch(type) {
            case 0: // constant
                return value;
            case 1: // register
                if (value < 0 || value >= reg_amount) throw runtime_error("Condition register index out of range");
                return regs[value];
            case 2: // ALU port
                if (value < 0 || value > 2) throw runtime_error("Condition ALU index out of range");
                return alu[value];
            case 3: // flag
                switch(value) {
                    case 1: return alu_trigger;
                    case 2: return *alu_zf;
                    case 3: return *alu_nf;
                    case 4: return *alu_of;
                    case 5: return halted;
                    default: throw runtime_error("Invalid flag code in condition: " + to_string(value));
                }
            case 4: // PC
                return pc;
            default:
                throw runtime_error("Unknown condition operand type: " + to_string(type));
        }
    };

    int lhs_val = compute_value(instr.cond1_type, instr.cond1);
    int rhs_val = compute_value(instr.cond2_type, instr.cond2);

    if (strcmp(instr.comp, "eq") == 0) return lhs_val == rhs_val;
    if (strcmp(instr.comp, "ne") == 0) return lhs_val != rhs_val;
    if (strcmp(instr.comp, "lt") == 0) return lhs_val < rhs_val;
    if (strcmp(instr.comp, "le") == 0) return lhs_val <= rhs_val;
    if (strcmp(instr.comp, "gt") == 0) return lhs_val > rhs_val;
    if (strcmp(instr.comp, "ge") == 0) return lhs_val >= rhs_val;

    return false;
}

int Cpu::exec_line(const Instruction& instr) {
    if (halted) {
        return 0; // CPU is halted; ignore instruction
    }
    int src_val = 0;
    
    // Ensure any pending ALU op is complete before reading *any* source.
    update_alu(); 

    // --- 1. Read Source (Fetch Data) ---
    switch (instr.source_type) {
    case 0: // constant
        src_val = instr.source_value;
        break;
    case 1: // register (R)
        if (instr.source_value < 0 || instr.source_value >= reg_amount)
            throw out_of_range("Source register index out of range: " + to_string(instr.source_value));
        src_val = regs[instr.source_value];
        break;
    case 2: // ALU port (A0 result, A1/A2 operands)
        if (instr.source_value < 0 || instr.source_value > 2)
            throw out_of_range("ALU source index out of range: " + to_string(instr.source_value));
        src_val = alu[instr.source_value];
        break;
        
    case 3: // flag
        switch(instr.source_value) {
            case 1: // alu flag (trigger)
                src_val = alu_trigger;
                break;
            case 2: // zero flag
                src_val = *alu_zf;
                break;
            case 3: // neg flag
                src_val = *alu_nf;
                break;
            case 4: // overflow flag
                src_val = *alu_of;
                break;
            default:
                throw runtime_error("Unknown flag source value: " + to_string(instr.source_value));
        }
        break;
    case 4: // program counter (PC)
        src_val = pc;
        break;
    default:
        throw runtime_error("Unknown source type: " + to_string(instr.source_type));
    }

    // --- 2. Write Destination (Transport Data) ---
    switch (instr.dest_type) {
    case 0: // Discard (Constant, NOP)
        break; 
    case 1: // register (R)
        if (instr.dest_value < 0 || instr.dest_value >= reg_amount)
            throw out_of_range("Dest register index out of range: " + to_string(instr.dest_value));
        regs[instr.dest_value] = src_val; 
        break;
    case 2: // ALU port (A0/A1/A2) - WRITE
        if (instr.dest_value < 1 || instr.dest_value > 2) {
            throw out_of_range("ALU dest port must be A1 or A2: " + to_string(instr.dest_value));
        }
        alu[instr.dest_value] = src_val;
        update_alu();
        break;
        
    case 3: // ALU flag (set trigger 'AF')
        switch(instr.dest_value) {
            case 1: // alu flag (AF)
                alu_trigger = src_val;
                update_alu();
                break;
            case 5: // halt flag (HF)
                if (src_val != 0) {
                    halted = 1;
                }
                break;
            default:
                throw runtime_error("Unknown or read-only flag destination value: " + to_string(instr.dest_value));
        }
        break;
        
    case 4: // program counter (PC - jump)
        pc = src_val;
        increment_pc = false;
        break;
    default:
        throw runtime_error("Unknown dest type: " + to_string(instr.dest_type));
    }
    // DEBUG: print_register_file();
    return 0;
}

Cpu::Cpu(int reg, int bus_) {
    reg_amount = reg;
    bus_amount = bus_;
    regs.resize(reg_amount, 0);
    bus.resize(bus_amount, 0);
}

int Cpu::exec_prog(const vector<Instruction>& prog) {
    // DEBUG
    cout << "--- Starting TTA Simulation ---" << endl;
    while (pc >= 0 && pc < static_cast<int>(prog.size())) {
        const Instruction& instr = prog[pc];
        
        bool execute = true;

        // Conditional Check Block
        if (instr.comp[0] != '\0') {
            if (!check_condition(instr)) {
                // Condition failed: skip execution
                cout << "PC " << pc << ": Condition FAILED. Skipping." << endl;
                execute = false;
            } else {
                // Condition met: proceed to execution
                cout << "PC " << pc << ": Condition MET. Executing." << endl;
            }
        }

        if (execute) {
            exec_line(instr);
        }

        // PC increment logic
        if (increment_pc) {
            pc++;
        }
        else {
            increment_pc = true; // reset flag after a successful jump
        }

        // Ensure we don’t read past program memory (important after jumps)
        if (pc >= static_cast<int>(prog.size())) break;
        
        print_register_file(); // Print state after every cycle
    }
    cout << "--- Program End ---" << endl;
    print_register_file();
    return 0;
}

void Cpu::print_register_file() {
    cout << "\n[ RF State ] ";
    for (int i = 0; i < reg_amount; ++i) {
        cout << "R" << i << "=" << regs[i] << (i < reg_amount - 1 ? " | " : "");
    }
    cout << endl << "[ ALU State ]";
    for (int i = 0; i < 3; ++i) {
        cout << " ALU" << i << "=" << alu[i];
    }
        cout << endl << " PC: " << pc << endl;
    cout << "ALU flags: ZF=" << *alu_zf << " NF=" << *alu_nf << " OF=" << *alu_of;
    cout << endl << "Halted: " << halted;
    cout << "\n" << endl;
}

int run_prog(const vector<RawInstruction>& prog_raw) {
    // DEBUG
    try {
        Cpu x = Cpu(NUM_REGISTERS_SAMPLE, BUS_COUNT_SAMPLE);
        // decode_program now returns vector<Instruction>
        vector<Instruction> x_decoded = decode_program(prog_raw);
        x.exec_prog(x_decoded);
    }
    catch (const runtime_error& e) {
        cerr << "Runtime Error: " << e.what() << endl;
        return 1;
    }
    catch (const out_of_range& e) {
        cerr << "Out of Range Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}

/*void test() {
    // Build program as assembly lines (human readable)
    std::vector<std::string> asm_lines = {
        "5 A1",
        "5 A2",
        "2 AF",
        "ZF R0",
        "5 PC ZF == 1",
        "10 R1 ZF == 0",
        "20 R2 ZF == 1",
        "R2 A1",
        "R2 A2",
        "1 AF",
        "30 R3 ZF == 0",
        "1 HF"
    };

    // Parse assembly into RawInstruction vector
    auto prog_raw = parse_program_lines(asm_lines);

    // Run program (parse->encode->decode pipeline happens inside run_prog)
    run_prog(prog_raw);
    return;
}*/
