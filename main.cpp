#include "cpu.hpp"
#include "computer.hpp"

using namespace std;

Computer c(1000000, NUM_REGISTERS_SAMPLE, BUS_COUNT_SAMPLE);

int main(){
    c.put_program(sample_program, 0);
    c.run_from_ram(0);
    return 0;
}

// Include implementation units so compiling only main.cpp supplies all definitions
#include "cpu.cpp"
#include "computer.cpp"

