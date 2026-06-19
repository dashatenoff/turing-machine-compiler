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

    std::map<long, char> getCells() const { return cells; }

private:

    std::map<long, char> cells;

    long head;

    char blankSymbol;
};


