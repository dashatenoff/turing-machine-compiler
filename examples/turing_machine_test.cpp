#include "tm_program.h"
#include "rule.h"
#include "turing_machine.h"

#include <iostream>

int main()
{
    TMProgram program;

    program.startState = "q0";

    Rule rule;

    rule.currentState = "q0";
    rule.readTape1 = '1';
    rule.readTape2 = '_';

    rule.writeTape1 = '0';
    rule.writeTape2 = '_';

    rule.moveTape1 = Move::Stay;
    rule.moveTape2 = Move::Stay;

    rule.nextState = "HALT";

    program.rules.push_back(rule);

    TuringMachine tm(program);

    tm.loadTape1("1");

    std::cout << tm.getTape1().read() << std::endl;

    tm.step();

    std::cout << tm.getTape1().read() << std::endl;

    return 0;
}