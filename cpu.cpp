#include "cpu.hpp"

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


string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

Instruction convert_line(const RawInstruction& line_raw) {
    Instruction prog = { 0, 0, 0, 0, "", "", "" }; // Initialize new condition fields
    
    // --- CONDITION PARSING ---
    if (!line_raw.condition.empty()) {
        string lhs, rhs, op;
        
        // NOTE: Use the correctly ordered array (ops_ordered)
        for (int i = 0; i < 6; ++i) { 
            // Find the operator in the condition string
            size_t pos = line_raw.condition.find(ops_ordered[i]);
            
            if (pos != string::npos){
                op = ops_ordered[i];
                
                // Extract LHS (substring from start up to the operator)
                lhs = line_raw.condition.substr(0, pos);
                
                // Extract RHS (substring from after the operator to the end)
                rhs = line_raw.condition.substr(pos + op.size());
                
                prog.cond1 = trim(lhs);
                prog.cond2 = trim(rhs);
                prog.comp = ops_map[op];
                break;
            }
        }
        // Handle case where condition is present but no known operator is found (optional error check)
        if (op.empty()) {
            throw runtime_error("Conditional statement '" + line_raw.condition + "' contains no valid comparison operator.");
        }
    }
    
    // --- SOURCE PARSING ---
    if (all_of(line_raw.src.begin(), line_raw.src.end(), ::isdigit) && !line_raw.src.empty()) {
        prog.source_type = 0; // number
        prog.source_value = stoi(line_raw.src);
    }
    else if (line_raw.src == "PC") {
        prog.source_type = 4; // program counter
        prog.source_value = 0;
    }
    // R# (Register)
    else if (line_raw.src[0] == 'R' && line_raw.src.size() > 1 && isdigit(line_raw.src[1])) {
        prog.source_type = 1; // register
        prog.source_value = stoi(line_raw.src.substr(1));
    }
    // #F (Flag: AF, ZF, NF, OF, HF)
    else if (line_raw.src.size() > 1 && line_raw.src[1] == 'F') { 
        prog.source_type = 3; // flag
        prog.source_value = 0;
        switch(line_raw.src[0]) {
            case 'A': prog.source_value = 1; break; // alu flag (trigger state)
            case 'Z': prog.source_value = 2; break; // zero flag
            case 'N': prog.source_value = 3; break; // neg flag
            case 'O': prog.source_value = 4; break; // overflow flag
            case 'H': prog.source_value = 5; break; // halt flag
            default: throw runtime_error("unknown flag symbol '" + line_raw.src + "'");
        }
    }
    // A# (ALU port)
    else if (line_raw.src[0] == 'A' && line_raw.src.size() > 1 && isdigit(line_raw.src[1])) {
        prog.source_type = 2; // ALU port
        prog.source_value = stoi(line_raw.src.substr(1));
    }
    else {
        throw runtime_error("unknown symbol '" + line_raw.src + "' in source");
    }

    // --- DESTINATION PARSING ---
    if (line_raw.dest == "PC") {
        prog.dest_type = 4; // program counter
        prog.dest_value = 0;
    }
    else if (line_raw.dest == "AF") { // Only AF can be a destination (to set the trigger)
        prog.dest_type = 3; // ALU flag (trigger)
        prog.dest_value = 1; // alu flag
    }
    else if (line_raw.dest == "HF") { // halt flag
        prog.dest_type = 3; // ALU flag (trigger)
        prog.dest_value = 5; // alu flag
    }
    else if (line_raw.dest[0] == 'R' && line_raw.dest.size() > 1 && isdigit(line_raw.dest[1])) {
        prog.dest_type = 1; // register
        prog.dest_value = stoi(line_raw.dest.substr(1));
    }
    else if (line_raw.dest[0] == 'A' && line_raw.dest.size() > 1 && isdigit(line_raw.dest[1])) {
        prog.dest_type = 2; // ALU port
        prog.dest_value = stoi(line_raw.dest.substr(1));
    }
    else if (all_of(line_raw.dest.begin(), line_raw.dest.end(), ::isdigit) && !line_raw.dest.empty()) {
        prog.dest_type = 0; // Discard/NOP
        prog.dest_value = 0;
    }
    else {
        throw runtime_error("unknown symbol '" + line_raw.dest + "' in destination");
    }

    return prog;
}
vector<Instruction> decode_program(const vector<RawInstruction>& prog_raw) {
    vector<Instruction> prog_decoded;
    prog_decoded.reserve(prog_raw.size()); 

    for (const auto& raw_instr : prog_raw) {
        prog_decoded.push_back(convert_line(raw_instr));
    }

    return prog_decoded;
}


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

int Cpu::get_flag_value(const string& flag_name) const {
    if (flag_name == "ZF") return *alu_zf;
    if (flag_name == "NF") return *alu_nf;
    if (flag_name == "OF") return *alu_of;
    // AF (ALU Trigger Flag) could also be checked, but usually ZF/NF/OF are the control flags
    // For simplicity, we'll only allow ZF, NF, OF here.
    throw runtime_error("Unknown flag name in condition: " + flag_name);
}

bool Cpu::check_condition(const Instruction& instr) {
    if (instr.comp.empty()) {
        return true; // No condition, always execute
    }
    
    // 1. Get LHS (Flag Value)
    int lhs_val = get_flag_value(instr.cond1);
    
    // 2. Get RHS (Expected Value - should be 0 or 1)
    int rhs_val;
    try {
        rhs_val = stoi(instr.cond2);
    } catch (...) {
        throw runtime_error("Condition RHS must be a constant integer (0 or 1). Found: " + instr.cond2);
    }

    // 3. Evaluate comparison based on the operator string
    if (instr.comp == "eq") return lhs_val == rhs_val;
    if (instr.comp == "ne") return lhs_val != rhs_val;
    
    // For flags, LT/LE/GT/GE usually don't make sense, but we'll include them for completeness.
    if (instr.comp == "lt") return lhs_val < rhs_val;
    if (instr.comp == "le") return lhs_val <= rhs_val;
    if (instr.comp == "gt") return lhs_val > rhs_val;
    if (instr.comp == "ge") return lhs_val >= rhs_val;
    
    // Should not be reached if parser works correctly
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
    print_register_file();
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
    while (pc >= 0 && pc < prog.size()) {
        const Instruction& instr = prog[pc];
        
        bool execute = true;

        // --- NEW: Conditional Check Block ---
        if (!instr.comp.empty()) {
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
        if (pc >= prog.size()) break;
        
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
    cout << endl << "ALU trigger: " << alu_trigger << " PC: " << pc << endl;
    cout << "ALU flags: ZF=" << *alu_zf << " NF=" << *alu_nf << " OF=" << *alu_of;
    cout << endl << "Halted: " << halted;
    cout << "\n" << endl;
}

int run_prog(const vector<RawInstruction>& prog_raw) {
    // DEBUG
    try {
        Cpu x = Cpu(NUM_REGISTERS_SAMPLE, BUS_COUNT_SAMPLE);
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

void test() {
    run_prog(sample_program);
    return;
}
