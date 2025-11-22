#include <iostream>
#include <vector>

#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"

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
        else if (tok[0] == "run") {
            c.run_from_ram(0);
        }
        else if (tok[0] == "load") {
            if (tok[1] == "sample"){
                c.put_program(sample_program, stoi(tok[2]));
            }
            else {
                cout << "Unknown program: " << tok[1] << endl;
                break;
            }
            cout << "Program loaded into memory." << endl;
        }
        else {
            cout << "Unknown command: " << tok[0] << endl;
        }
        cout << "> ";
    }
}

