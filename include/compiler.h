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

private:
    StateGenerator      gen_;
    std::vector<Rule>   rules_;
    std::string         tapeValue_;

    //обход AST
    // entry состояние с которого начать блок а возвращает состояние на котором блок закончил
    std::string compileBody(
            const std::vector<NodePtr>& body,
            const std::string& entry
    );

    std::string compileStatement(const Node& node,   const std::string& entry);
    std::string compileCommand  (const CommandNode&, const std::string& entry);
    std::string compileLoop     (const LoopNode&,    const std::string& entry);

    // entry это состояние входа в макрос
    void emitInvert(const std::string& entry, const std::string& exit);
    void emitAdd   (const std::string& entry, const std::string& exit);

    void addRule( //
            const std::string& cur,
            char r1, char r2,
            char w1, char w2,
            Move m1, Move m2,
            const std::string& next
    );
};