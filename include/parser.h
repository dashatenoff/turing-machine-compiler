#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include "token.h"
#include "ast.h"

//parser строит AST из списка токенов

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    //метод возвращает корень дерева
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens_;
    std::size_t        pos_;
    std::unique_ptr<ProgramNode> parseProgram();
    NodePtr                      parseStatement();
    std::unique_ptr<LoopNode>    parseLoop();
    std::unique_ptr<CommandNode> parseCommand();

    const Token& peek() const;
    const Token& advance();
    bool         check(TokenType type) const;
    bool         isAtEnd() const;

    //съедает токен если он нужного типа иначе бросает ошибку с номером строки
    const Token& expect(TokenType type, const std::string& where);
};