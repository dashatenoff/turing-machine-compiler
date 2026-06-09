#pragma once

#include <string>
#include <vector>
#include <memory>

//базовый узел AST
//чтобы добавить новый тип узла нужно создать новый struct наследник Node и добавить его в visit в Compiler

struct Node {
    int line = 0;   //для ошибок компилятора
    virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

//корень дерева  и есть вся программа
struct ProgramNode : Node {
    std::string          tapeValue;   // содержимое ленты
    std::vector<NodePtr> body;  //список команд или циклов
};

//чтообы добавить новую команду: только новый CommandType всё остальное не меняется
enum class CommandType {
    INVERT,
    ADD,
};

struct CommandNode : Node {
    CommandType command;
};

//условие цикла пока
enum class ConditionType {
    HAS_BIT,  // есть_бит — tape1.read() != '_'
};

//цикл пока условие: ... конец
struct LoopNode : Node {
    ConditionType        condition;
    std::vector<NodePtr> body; // тело цикла
};