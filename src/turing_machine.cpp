#include "turing_machine.h"

TuringMachine::TuringMachine(const TMProgram& program){
    this->program = program;
    currentState = program.startState;
    steps = 0;
    loadTape1(program.tapeValue);
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

void TuringMachine::loadTape1(const std::string& input){
    tape1.reset();
    for (int i=0; i < input.size(); i++){
        tape1.write(input[i]);
        tape1.moveRight();
    }
    while (tape1.getPosition() > 0){
        tape1.moveLeft();
    }
}


void TuringMachine::loadTape2(const std::string& input){
    tape2.reset();
    for (int i=0; i < input.size(); i++){
        tape2.write(input[i]);
        tape2.moveRight();
    }
    while (tape2.getPosition() > 0){
        tape2.moveLeft();
    }
}

std::string TuringMachine::getCurrentState() const {
    return currentState;
}