#pragma once

#include <string>
#include <vector>
#include "cpu.hpp"

// Convert a RawInstruction into a space-delimited machine-code string
Instruction convert_line(const RawInstruction& line_raw);

// Encode an entire program (RawInstruction vector) into machine-code lines
std::vector<Instruction> decode_program(const std::vector<RawInstruction>& prog_raw);
