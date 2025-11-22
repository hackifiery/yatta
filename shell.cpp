#include <iostream>
#include <vector>

#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include <fstream>
#include <iterator>

using namespace std;

std::vector<std::string> splitString(const std::string& s, char delimiter) {
    // helpers for parsing
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string::npos) {
        token = s.substr(start, end - start);
        tokens.push_back(token);
        start = end + 1;
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start)); // Add the last token
    return tokens;
}

string raw;
vector<string> tok;

void shell() {
    Computer c(128, 8, 1);
    cout << "> ";
    while (getline(cin, raw)) {
        tok = splitString(raw, ' ');
        if (tok[0] == "exit") {
            break;
        }
        else if (tok[0] == "assemble") {
            // assemble <src.asm> <out.bin>
            if (tok.size() < 3) {
                cout << "Usage: assemble <src.asm> <out.bin>" << endl;
            } else {
                string src = tok[1];
                string out = tok[2];
                try {
                    // read source file lines
                    ifstream ifs(src);
                    if (!ifs) { cout << "Failed to open source: " << src << endl; }
                    vector<string> lines;
                    string line;
                    while (std::getline(ifs, line)) lines.push_back(line);

                    // parse and assemble
                    auto raw = parse_program_lines(lines);
                    auto encoded = decode_program(raw);
                    vector<Instruction> prog;
                    prog.reserve(encoded.size());
                    for (auto &s : encoded) prog.push_back(string_to_instr(s));

                    // write binary file
                    ofstream ofs(out, ios::binary);
                    if (!ofs) { cout << "Failed to open output file: " << out << endl; }
                    for (const auto &instr : prog) {
                        ofs.write(reinterpret_cast<const char*>(&instr), sizeof(instr));
                    }
                    cout << "Assembled " << src << " -> " << out << " (" << prog.size() << " instr)" << endl;
                } catch (const std::exception &e) {
                    cout << "Assemble error: " << e.what() << endl;
                }
            }
        }
        else if (tok[0] == "runbin") {
            // runbin <file.bin> [start]
            if (tok.size() < 2) {
                cout << "Usage: runbin <file.bin> [start_address]" << endl;
            } else {
                string file = tok[1];
                int start = 0;
                if (tok.size() >= 3) start = stoi(tok[2]);
                // read binary file into memory and run
                ifstream ifs(file, ios::binary);
                if (!ifs) { cout << "Failed to open binary: " << file << endl; }
                vector<uint8_t> buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                if (buf.empty()) { cout << "Empty or unreadable binary: " << file << endl; }
                // create Computer sized to file
                int mem_bytes = static_cast<int>(buf.size());
                Computer runner(mem_bytes, 8, 1);
                // copy bytes into runner.memory
                runner.memory.assign(buf.begin(), buf.end());
                try {
                    runner.run_from_ram(start);
                } catch (const std::exception &e) {
                    cout << "Runtime error: " << e.what() << endl;
                }
            }
        }
        else if (tok[0] == "asrun") {
            // asrun <src.asm> - assemble in-memory and run immediately
            if (tok.size() < 2) {
                cout << "Usage: asrun <src.asm>" << endl;
            } else {
                string src = tok[1];
                try {
                    ifstream ifs(src);
                    if (!ifs) { cout << "Failed to open source: " << src << endl; }
                    vector<string> lines;
                    string line;
                    while (std::getline(ifs, line)) lines.push_back(line);
                    auto raw = parse_program_lines(lines);
                    // assemble to Instruction vector
                    auto encoded = decode_program(raw);
                    vector<Instruction> prog;
                    prog.reserve(encoded.size());
                    for (auto &s : encoded) prog.push_back(string_to_instr(s));
                    // create computer sized to program and load
                    int mem_bytes = static_cast<int>(prog.size() * sizeof(Instruction));
                    Computer runner(mem_bytes, 8, 1);
                    runner.put_program(prog, 0);
                    runner.run_from_ram(0);
                } catch (const std::exception &e) {
                    cout << "As-run error: " << e.what() << endl;
                }
            }
        }
        else {
            cout << "Unknown command: " << tok[0] << endl;
        }
        cout << "> ";
    }
}

