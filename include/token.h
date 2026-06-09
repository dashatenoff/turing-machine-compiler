#pragma once

#include <string>

//Типы токенов
enum class TokenType {

    // ── Ключевые слова ────────────────────────
    TAPE,   // "лента"
    WHILE,// "пока"
    END, // "конец"
    HAS_BIT,
    CMD_INVERT,
    CMD_ADD,
    COLON, // :
    STRING,    //произвольная строка
    END_OF_FILE//конец входного потока
};

//Токен пара (тип, значение) value заполняется только для STRING а для остальных токенов value пустая строка
struct Token {
    TokenType   type;
    std::string value;
    int         line;//номер строки в исходном файле (для ошибок)
};