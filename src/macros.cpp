#include "macros.h"

TMProgram emitInvert() {
    TMProgram program;

    program.startState = "q0";
    program.rules.push_back({"q0", '0', '_', '0', '1', Move::Right, Move::Right, "q0"});
    program.rules.push_back({"q0", '1', '_', '1', '0', Move::Right, Move::Right, "q0"});
    program.rules.push_back({"q0", '_', '_', '_', '_', Move::Stay, Move::Stay, "HALT"});
    return program;
}