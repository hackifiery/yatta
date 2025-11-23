#include <iostream>
#include <thread>
#include <fstream>
#include <iterator>
#include <string>
#include <stdexcept>
#include <cctype>

// --- OS DETECTION AND INCLUDES ---
#if !defined(_WIN32) && !defined(_WIN64)
#define os 1 // Unix-like systems (Linux, macOS)
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h> // for perror
#else
#define os 2 // Windows
#include <windows.h> // for Sleep
#include <conio.h>   // for _kbhit and _getch
#endif

#include "cpu.hpp"
#include "assembler.hpp"
#include "computer.hpp"
#include "parser.hpp"
#include "shell.hpp"

using namespace std;

// --- FRONT PANEL IMPLEMENTATION (Platform Specific) ---

#if os == 1
void frontPanel(Computer& comp) {
    struct termios old_settings, new_settings;

    // Save current terminal settings
    if (tcgetattr(STDIN_FILENO, &old_settings) == -1) {
        perror("tcgetattr");
        return;
    }

    // Set non-canonical, non-echo mode (raw input)
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 0;    // return immediately from read if no data
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) == -1) {
        perror("tcsetattr");
        return;
    }

    bool running = true;
    while (running) {
        // Clear screen and reset cursor (POSIX ANSI escape codes)
        cout << "\033[2J\033[H";

        comp.cpu.print_register_file();
        cout << "\n[ BUS State ] ";
        for (int i = 0; i < comp.bus_num; ++i) {
            cout << "B" << i << "=" << comp.cpu.bus[i] << (i < comp.bus_num - 1 ? " | " : "");
        }
        cout << endl;
        cout << "\nPress 'q' to quit front panel." << endl;
        cout.flush();

        // --- Non-blocking Input with Select (POSIX) ---
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200 ms refresh

        int sel = select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv);
        if (sel == -1) {
            // select error, break
            break;
        }
        else if (sel > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
            char ch;
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            if (n == 1) {
                if (ch == 'q' || ch == 'Q') {
                    running = false;
                }
            }
        }
        // else timeout -> loop and refresh display
    }

    // Restore terminal settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) == -1) {
        perror("tcsetattr restore");
    }
    return;
}
#elif os == 2 // Windows Implementation
void frontPanel(Computer& comp) {
    bool running = true;
    while (running) {
        // Clear screen (Windows command)
        system("cls");

        comp.cpu.print_register_file();
        cout << "\n[ BUS State ] ";
        for (int i = 0; i < comp.bus_num; ++i) {
            cout << "B" << i << "=" << comp.cpu.bus[i] << (i < comp.bus_num - 1 ? " | " : "");
        }
        cout << endl;
        cout << "\nPress 'q' to quit front panel." << endl;
        cout.flush();

        // --- Non-blocking Input with _kbhit (Windows) ---
        if (_kbhit()) {
            char ch = _getch(); // Reads the character without requiring Enter
            if (ch == 'q' || ch == 'Q') {
                running = false;
            }
        }

        // Delay for screen refresh (200 ms)
        Sleep(200);
    }
    // No terminal state restoration needed for _kbhit/system("cls")
}
#endif

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

