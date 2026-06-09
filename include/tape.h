#pragma once

#include <map>
#include <string>

class Tape {
public:

    Tape(char blank = '_');

    char read() const;

    void write(char symbol);

    void moveLeft();

    void moveRight();

    long getPosition() const;

    void reset();

    std::string toString() const;

private:

    std::map<long, char> cells;

    long head;

    char blankSymbol;
};


