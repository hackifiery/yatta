#include "converter.hpp"
#include <sstream>
#include <stdexcept>

using namespace std;

RawInstruction parse_raw_instruction(const string& line) {
    // Tokenize by whitespace
    istringstream iss(line);
    vector<string> tokens;
    string t;
    while (iss >> t) tokens.push_back(t);

    if (tokens.size() < 2) {
        throw runtime_error("parse_raw_instruction: line must have at least source and dest");
    }

    string src = trim(tokens[0]);
    string dest = trim(tokens[1]);

    string condition = "";
    if (tokens.size() > 2) {
        // If the third token is a placeholder '?', skip it
        size_t start_idx = 2;
        if (tokens[2] == "?") start_idx = 3;
        // Join remaining tokens with spaces
        for (size_t i = start_idx; i < tokens.size(); ++i) {
            if (!condition.empty()) condition += " ";
            condition += tokens[i];
        }
        condition = trim(condition);
    }

    return RawInstruction{ src, dest, condition };
}

vector<RawInstruction> parse_program_lines(const vector<string>& lines) {
    vector<RawInstruction> out;
    out.reserve(lines.size());
    for (const auto& l : lines) {
        string s = trim(l);
        if (s.empty()) continue; // skip blank lines
        out.push_back(parse_raw_instruction(s));
    }
    return out;
}
