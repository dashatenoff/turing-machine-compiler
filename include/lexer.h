#pragma once

#include <string>
#include <vector>
#include "token.h"

//Lexer разбивает исходный текст на токены
//чтобы добавить новое ключевое слово нужно добавить TokenType в token.h и добавить строку в keywords_ ниже

class Lexer {
public:
    explicit Lexer(const std::string& source);
    //возвращает все токены
    std::vector<Token> tokenize();

private:
    std::string source_;
    std::size_t pos_;
    int         line_;
    void        skipWhitespace();
    Token       readWord();
    Token       readString();
    char        peek() const;
    char        advance();
    bool        isAtEnd() const;
};