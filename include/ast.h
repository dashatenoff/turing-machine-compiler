#pragma once

#include <string>
#include <vector>
#include <memory> //юник

//базовый узел AST типа роодительский
//чтобы добавить новый тип узла нужно создать новый struct наследник Node и добавить его в visit в Compiler

struct Node {
    int line = 0;   //для ошибок компилятора
    virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

//корень дерева  и есть вся программа
struct ProgramNode : Node {
    std::string          tapeValue;   //содержимое ленты 1 (ввод)
    std::vector<NodePtr> body;  //список команд или циклов указатели тк боди может состоять из разных типов
};//каждый узел хранит список своих детей через NodePtr

//чтообы добавить новую команду: только новый CommandType всё остальное не меняется
enum class CommandType {//parser его только записывает при создании узла сompiler его потом читает чтобы решить что делат
    INVERT,
    ADD,
};

struct CommandNode : Node {
    CommandType command;
};

//условие цикла пока
enum class ConditionType {
    HAS_BIT,
};

//цикл пока условие: ... конец
struct LoopNode : Node {//каждый LoopNode несёт своё собственное тело поэтому циклы можно вкладывать друг в друга
    ConditionType        condition;
    std::vector<NodePtr> body; // тело цикла
};