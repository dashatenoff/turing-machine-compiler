#include "macros.h"
#include "turing_machine.h"

#include <iostream>

int main()
{
    TMProgram program = emitInvert();

    TuringMachine tm(program);

    tm.loadTape1("1011");

    tm.run();

    std::cout << tm.getTape2().toString() << std::endl;

    return 0;
}