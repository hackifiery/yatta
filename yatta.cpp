#include "cpu.hpp"
#include "computer.hpp"
#include "shell.hpp"
#include "parser.hpp"

using namespace std;

int main(){
    shell();
}

// Include implementation units so building only main.cpp provides all definitions
#include "cpu.cpp"
#include "computer.cpp"
#include "shell.cpp"
#include "parser.cpp"
#include "assembler.cpp"