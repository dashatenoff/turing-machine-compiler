#pragma once

#include <vector>
#include <string>
#include "ast.h"
#include "rule.h"
#include "tm_program.h"
#include "state_generator.h"

//Compiler обходит AST и генерирует TMProgram

class Compiler {
public:
    TMProgram compile(const ProgramNode& program);
    StateGenerator      gen_;

    // entry состояние с которого начать блок а возвращает состояние на котором блок закончил
    //каждый метод возвращает имя состояния выхода exitState из обработанного блока (узлов)
    // и также каждый метод принимает entry потом генериурет правила и возвращает exit который станет
    // entry следующего метода
    std::string compileBody(const std::vector<NodePtr>& body, const std::string& entry);

    void emitInvert(const std::string& entry, const std::string& exit);//методы чтобы добавить в rules_ правила через эдрул
    void emitAdd   (const std::string& entry, const std::string& exit);

    void addRule(
            const std::string& cur,
            char r1, char r2,
            char w1, char w2,
            Move m1, Move m2,
            const std::string& next
    );

private:
    std::vector<Rule>   rules_;
    std::string         tapeValue_;

};