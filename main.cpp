#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"
#include "converter.hpp"

using namespace std;

int main(){
    // shell();
    Computer c(64, 4, 1);
    RawInstruction inst = parse_program_lines({"1 HF ? R1 == 0"})[0];
    c.put_program({inst}, 0);
    c.run_from_ram(0);
    return 0;
}

// Include implementation units so building only main.cpp provides all definitions
#include "cpu.cpp"
#include "computer.cpp"
#include "shell.cpp"
#include "converter.cpp"