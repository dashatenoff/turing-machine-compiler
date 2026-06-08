#include <iostream>
#include "tape.h"

int main() {
    Tape tape;

    tape.write('1');
    tape.moveRight();
    tape.write('0');
    tape.moveLeft();
    std::cout << tape.read() << std::endl; // Output: 1
    tape.moveRight();
    std::cout << tape.read() << std::endl; // Output: 0

    return 0;
}