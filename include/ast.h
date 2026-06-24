#pragma once

#include <string>
#include <vector>
#include <memory> //юник

//базовый узел AST типа роодительский
//чтобы добавить новый тип узла нужно создать новый struct наследник Node и добавить его в visit в Compiler

class Compiler;//предварительное объявление тк аст включает компайл и наоборот

//virtual compile = 0 делает Node абстрактным тк нельзя создать Node напрямую
struct Node {
    int line = 0;//для ошибок компилятора
    virtual ~Node() = default;
    virtual std::string compile(Compiler& compiler, const std::string& entry) const = 0;
};

using NodePtr = std::unique_ptr<Node>;

//корень дерева  и есть вся программа
struct ProgramNode : Node {
    std::string          tapeValue;   //содержимое ленты 1 (ввод)
    std::vector<NodePtr> body;  //список команд или циклов указатели тк боди может состоять из разных типов
    std::string compile(Compiler& compiler, const std::string& entry) const override;
};//каждый узел хранит список своих детей через NodePtr

enum class CommandType {//parser его только записывает при создании узла сompiler его
    // потом читает чтобы решить что делат
    INVERT,
    ADD,
};

//CommandNode сам знает что вызвать в compiler
struct CommandNode : Node {
    CommandType command;
    std::string compile(Compiler& compiler, const std::string& entry) const override;
};

//условие цикла пока
enum class ConditionType {
    HAS_BIT,
};

struct LoopNode : Node {//каждый LoopNode несёт своё собственное тело поэтому циклы можно вкладывать друг в друга
    ConditionType        condition;
    std::vector<NodePtr> body; // тело цикла
    std::string compile(Compiler& compiler, const std::string& entry) const override;
};