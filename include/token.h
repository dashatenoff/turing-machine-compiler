#pragma once

#include <string>

//типы токенов
enum class TokenType {
    TAPE, // "лента"
    WHILE,// "пока"
    END, // "конец"
    HAS_BIT,
    CMD_INVERT,
    CMD_ADD,
    COLON, // :
    STRING, //произвольная строка
    END_OF_FILE//конец входного потока
};

//токен пара (тип, значение) value заполняется только для STRING а для остальных токенов value пустая строка
struct Token {
    TokenType   type;
    std::string value;
    int         line;//номер строки в исходном файле (для ошибок)
};