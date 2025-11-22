#include "assembler.hpp"
#include "parser.hpp"
#include <sstream>
#include <stdexcept>
#include <cstring>

using namespace std;

string convert_line(const RawInstruction& line_raw) {
    Instruction prog = { 0, 0, 0, 0, {0}, {0}, {0}, -1, -1, 0, 0 }; // zero-init + cond types/values

    // --- CONDITION PARSING ---
    if (!line_raw.condition.empty()) {
        string lhs, rhs, op;
        
        for (int i = 0; i < 6; ++i) {
            size_t pos = line_raw.condition.find(ops_ordered[i]);
            if (pos != string::npos){
                op = ops_ordered[i];
                lhs = line_raw.condition.substr(0, pos);
                rhs = line_raw.condition.substr(pos + op.size());
                string lhs_trim = trim(lhs);
                string rhs_trim = trim(rhs);
                string comp_str = ops_map[op];
                // copy into fixed-size fields (ensure null termination)
                strncpy(prog.cond1, lhs_trim.c_str(), sizeof(prog.cond1)-1);
                prog.cond1[sizeof(prog.cond1)-1] = '\0';
                strncpy(prog.cond2, rhs_trim.c_str(), sizeof(prog.cond2)-1);
                prog.cond2[sizeof(prog.cond2)-1] = '\0';
                strncpy(prog.comp, comp_str.c_str(), sizeof(prog.comp)-1);
                prog.comp[sizeof(prog.comp)-1] = '\0';

                // Parse lhs and rhs into typed operands (reuse source parsing rules)
                auto parse_operand = [&](const string& tok)->pair<int,int>{
                    if (tok.empty()) throw runtime_error("empty condition operand");
                    if (all_of(tok.begin(), tok.end(), ::isdigit)) {
                        return {0, stoi(tok)}; // constant
                    }
                    if (tok == "PC") {
                        return {4, 0};
                    }
                    if (tok[0] == 'R' && tok.size() > 1 && isdigit(tok[1])) {
                        return {1, stoi(tok.substr(1))};
                    }
                    if (tok.size() > 1 && tok[1] == 'F') {
                        int v = 0;
                        switch(tok[0]) {
                            case 'A': v = 1; break;
                            case 'Z': v = 2; break;
                            case 'N': v = 3; break;
                            case 'O': v = 4; break;
                            case 'H': v = 5; break;
                            default: throw runtime_error("unknown flag symbol in condition: '" + tok + "'");
                        }
                        return {3, v};
                    }
                    if (tok[0] == 'A' && tok.size() > 1 && isdigit(tok[1])) {
                        return {2, stoi(tok.substr(1))};
                    }
                    throw runtime_error("unknown token in condition: '" + tok + "'");
                };

                auto p1 = parse_operand(lhs_trim);
                auto p2 = parse_operand(rhs_trim);
                // store typed operand info: both type and numeric value (mirror src/dest format)
                prog.cond1_type = p1.first;
                prog.cond1_value = p1.second;
                prog.cond2_type = p2.first;
                prog.cond2_value = p2.second;
                break;
            }
        }
        if (prog.comp[0] == '\0') {
            throw runtime_error("Conditional statement '" + line_raw.condition + "' contains no valid comparison operator.");
        }
    } else {
        // no condition
        prog.cond1_type = -1;
        prog.cond2_type = -1;
        prog.comp[0] = '\0';
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

    // Build a space-delimited "machine code" string. Fields:
    // source_type source_value dest_type dest_value
    // cond1_type cond1_value cond2_type cond2_value comp
    string out =
        to_string(prog.source_type) + " " + to_string(prog.source_value) + " " +
        to_string(prog.dest_type) + " " + to_string(prog.dest_value);

    if (prog.comp[0] != '\0') {
        out += " " + to_string(prog.cond1_type) + " " + to_string(prog.cond1_value) +
               " " + to_string(prog.cond2_type) + " " + to_string(prog.cond2_value) +
               " " + string(prog.comp);
    }

    return out;
}

vector<string> decode_program(const vector<RawInstruction>& prog_raw) {
    vector<string> prog_decoded;
    prog_decoded.reserve(prog_raw.size()); 

    for (const auto& raw_instr : prog_raw) {
        prog_decoded.push_back(convert_line(raw_instr));
    }

    return prog_decoded;
}


Instruction string_to_instr(const std::string& machine_code) {
    Instruction prog = { 0, 0, 0, 0, {0}, {0}, {0}, -1, -1, 0, 0 };

    // Tokenize by whitespace
    istringstream iss(machine_code);
    vector<string> tokens;
    string tok;
    while (iss >> tok) tokens.push_back(tok);

    auto get_int = [&](size_t idx, int def)->int {
        if (idx < tokens.size()) {
            try {
                return stoi(tokens[idx]);
            } catch (...) {
                throw runtime_error("string_to_instr: expected integer at token " + to_string(idx));
            }
        }
        return def;
    };

    prog.source_type = get_int(0, 0);
    prog.source_value = get_int(1, 0);
    prog.dest_type = get_int(2, 0);
    prog.dest_value = get_int(3, 0);

    // Machine-code format now requires typed condition fields when a condition is present.
    // Layout when condition present:
    // source_type source_value dest_type dest_value cond1_type cond1_value cond2_type cond2_value comp
    // If no condition is present the line may end after the 4 header tokens.
    string comp;
    if (tokens.size() > 4) {
        // Expect typed condition fields: cond1_type cond1_value cond2_type cond2_value comp
        if (tokens.size() < 9) {
            throw runtime_error("string_to_instr: typed machine-code must contain 9 tokens when condition is present");
        }
        prog.cond1_type = get_int(4, -1);
        prog.cond1_value = get_int(5, 0);
        prog.cond2_type = get_int(6, -1);
        prog.cond2_value = get_int(7, 0);
        comp = tokens[8];
    } else {
        // no condition
        prog.cond1_type = -1;
        prog.cond2_type = -1;
        prog.cond1_value = 0;
        prog.cond2_value = 0;
        comp = string("");
    }

    // copy comparison into fixed-size char array (ensure null termination)
    strncpy(prog.comp, comp.c_str(), sizeof(prog.comp)-1);
    prog.comp[sizeof(prog.comp)-1] = '\0';

    return prog;
}
