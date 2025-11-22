#pragma once

#include <string>
#include <vector>
#include "cpu.hpp"

// Convert a RawInstruction into a space-delimited machine-code string
std::string convert_line(const RawInstruction& line_raw);

// Encode an entire program (RawInstruction vector) into machine-code lines
std::vector<std::string> decode_program(const std::vector<RawInstruction>& prog_raw);

// Convert a space-delimited machine-code string back into an Instruction
Instruction string_to_instr(const std::string& machine_code);
