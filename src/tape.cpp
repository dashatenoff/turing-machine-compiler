#include "tape.h"

Tape::Tape(char blank){
    head = 0;
    blankSymbol = blank;
}

void Tape::moveLeft(){
    head--;
}

void Tape::moveRight(){
    head++;
}

long Tape::getPosition() const {
    return head;
}

void Tape::write(char symbol){
    cells[head] = symbol;
}

char Tape::read() const{
    auto it = cells.find(head);
    if (it != cells.end()){
        return it->second;
    } else {
        return blankSymbol;
    }
}

void Tape::reset(){
    head = 0;
    cells.clear();
}

