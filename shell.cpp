#include <iostream>
#include <vector>
#include <thread>
#include <termios.h>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"
#include "parser.hpp"
#include "assembler.hpp"


using namespace std;

void frontPanel(Computer& comp){
    struct termios old_settings, new_settings;
    if (tcgetattr(STDIN_FILENO, &old_settings) == -1) {
        perror("tcgetattr");
        return;
    }
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 0;   // return immediately from read if no data
    new_settings.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) == -1) {
        perror("tcsetattr");
        return;
    }

    bool running = true;
    while (running) {
        // clear screen and show registers/buses
        cout << "\033[2J\033[H";
        comp.cpu.print_register_file();
        cout << "\n[ BUS State ] ";
        for (int i = 0; i < comp.bus_num; ++i) {
            cout << "B" << i << "=" << comp.cpu.bus[i] << (i < comp.bus_num - 1 ? " | " : "");
        }
        cout << endl;
        cout.flush();

        // wait for input (timeout) so the panel refreshes periodically
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200 ms refresh

        int sel = select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv);
        if (sel == -1) {
            // select error, break and restore terminal
            break;
        }
        else if (sel > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
            char ch;
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            if (n == 1) {
                if (ch == 'q' || ch == 'Q') {
                    running = false;
                }
                // consume any extra input on line (non-blocking)
                // optional: flush remaining bytes to avoid rapid repeats
                while (true) {
                    char junk;
                    ssize_t m = read(STDIN_FILENO, &junk, 1);
                    if (m <= 0) break;
                }
            }
        }
        // else timeout -> loop and refresh display
    }

    // restore terminal
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) == -1) {
        perror("tcsetattr restore");
    }
    return;
}

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
                    for (auto &s : encoded) prog.push_back(s);

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
        else if (tok[0] == "load") {
            // load <file.bin>
            if (tok.size() < 2) {
                cout << "Usage: load <file.bin> <start address>" << endl;
            }
            else {
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
                // copy bytes into c.memory
                c.memory.assign(buf.begin(), buf.end());
            }
        }
        else if (tok[0] == "run") {
            // run <start address>
            if (tok.size() < 2) {
                cout << "Usage: run <start address>" << endl;
                continue;
            }
            if (!(isdigit(tok[1][0]) || (tok[1][0] == '-' && tok[1].size() > 1 && isdigit(tok[1][1])))) {
                cout << "Invalid start address: " << tok[1] << endl;
                continue;
            }
            try {
                thread t(&Computer::run_from_ram, &c, stoi(tok[1]));
                t.detach();
            } catch (const std::exception &e) {
                cout << "Runtime error: " << e.what() << endl;
            }
        }
        else if (tok[0] == "fp") {
            frontPanel(c);
        }
        else {
            cout << "Unknown command: " << tok[0] << endl;
        }
        cout << "> ";
    }
}

