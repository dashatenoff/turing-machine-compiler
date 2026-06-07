#pragma once

#include "tape.h"
#include "tm_program.h"

class TuringMachine {
public:

    TuringMachine(const TMProgram& program);

    bool step();

    void run();

    bool halted() const;

    void reset();

    const Tape& getTape1() const; // константная ссылка(нельзя менять)
    const Tape& getTape2() const;

    long getsteps() const;

private:

    Tape tape1;
    Tape tape2;

    TMProgram program;

    std::string currentState;

    long steps;
};