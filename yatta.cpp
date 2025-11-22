#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"
#include "parser.hpp"

using namespace std;

int main(){
    // shell();

    // Build an assembly program (human-friendly). We'll assemble this, load into memory, and run it.
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

    // Parse assembly into RawInstruction
    auto prog_raw = parse_program_lines(asm_lines);

    // Allocate computer memory sized to hold the assembled instructions
    int instr_count = static_cast<int>(prog_raw.size());
    int mem_bytes = static_cast<int>(instr_count * sizeof(Instruction));
    Computer c(mem_bytes, 4, 1);

    // Assemble (inside put_program: decode_program -> string_to_instr) and load into memory
    c.put_program(prog_raw, 0);

    // Run the program from memory
    c.run_from_ram(0);
    return 0;
}

// Include implementation units so building only main.cpp provides all definitions
#include "cpu.cpp"
#include "computer.cpp"
#include "shell.cpp"
#include "parser.cpp"
#include "assembler.cpp"