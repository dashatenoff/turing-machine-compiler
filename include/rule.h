#pragma once

#include <string>

enum class Move {
    Left,
    Right,
    Stay
};

struct Rule {

    std::string currentState;

    char readTape1;
    char readTape2;

    char writeTape1;
    char writeTape2;

    Move moveTape1;
    Move moveTape2;

    std::string nextState;
};