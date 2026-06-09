#include "tm_format.h"

#include <fstream>

char moveToChar(Move move) {
    switch (move) {
        case Move::Left:
            return 'L';
        case Move::Right:
            return 'R';
        case Move::Stay:
            return 'S';
    }
    return 'S'; 
}

Move charToMove(char c) {
    switch (c) {
        case 'L':
            return Move::Left;
        case 'R':
            return Move::Right;
        case 'S':
            return Move::Stay;
    }
    return Move::Stay;
}

bool TMFormat::save(const std::string& filename, const TMProgram& program) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    file << "start=" << program.startState << "\n";
    for (const Rule& rule : program.rules) {
        file << rule.currentState << " "
            << rule.readTape1 << " "
            << rule.readTape2 << " -> "
            << rule.writeTape1 << " "
            << rule.writeTape2 << " "
            << moveToChar(rule.moveTape1) << " "
            << moveToChar(rule.moveTape2) << " "
            << rule.nextState << "\n";
    }
    return true;
}

bool TMFormat::load(const std::string& filename, TMProgram& program) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    program.rules.clear();
    std::string line;
    std::getline(file, line);
    
    file >> program.startState;


}