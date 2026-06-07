#include "turing_machine.h"

TuringMachine::TuringMachine(const TMProgram& program){
    this->program = program;
    currentState = program.startState;
    steps = 0;
}

bool TuringMachine::step() {
    char s1 = tape1.read();
    char s2 = tape2.read();

    if(currentState == "HALT"){
        return false;
    }

    for (const Rule& rule : program.rules){
        if (rule.readTape1 == s1 && rule.readTape2 == s2 && rule.currentState == currentState){
            tape1.write(rule.writeTape1);
            tape2.write(rule.writeTape2);
            currentState = rule.nextState;

            switch (rule.moveTape1){
                case Move::Left:
                    tape1.moveLeft();
                    break;

                case Move::Right:
                    tape1.moveRight();
                    break;
                
                case Move::Stay:
                    break;
            }

            switch (rule.moveTape2){
                case Move::Left:
                    tape2.moveLeft();
                    break;

                case Move::Right:
                    tape2.moveRight();
                    break;
                
                case Move::Stay:
                    break;
            }
        steps++;
        return true;
        }
    }
    return false;
}

void TuringMachine::run() {
    while (step()){
    }
}

bool TuringMachine::halted() const {
    return currentState == "HALT";
}

void TuringMachine::reset() {
    tape1.reset();
    tape2.reset();
    currentState = program.startState;
    steps = 0;
}

const Tape& TuringMachine::getTape1() const {
    return tape1;
}

const Tape& TuringMachine::getTape2() const {
    return tape2;
}

long TuringMachine::getsteps() const {
    return steps;
}