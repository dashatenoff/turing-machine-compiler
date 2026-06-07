#pragma once

#include "tape.h"
#include "tm_program.h"

class TuringMachine {
public:

    explicit TuringMachine(const TMProgram& program);

    bool step();

    void run();

    bool halted() const;

    void reset();

private:

    Tape tape1;
    Tape tape2;

    TMProgram program;

    std::string currentState;

    long steps;
};