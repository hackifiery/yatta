#pragma once
#include <string>
#include <vector>
#include "cpu.hpp"

// Parse a single assembly line (e.g. "R3 R1 ? ZF == 1") into a RawInstruction.
// Throws std::runtime_error on parse errors.
RawInstruction parse_raw_instruction(const std::string& line);

// Convenience: parse multiple lines.
std::vector<RawInstruction> parse_program_lines(const std::vector<std::string>& lines);
