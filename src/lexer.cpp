#include "lexer.h"
#include <unordered_map>
#include <stdexcept>

// Таблица ключевых слов

static const std::unordered_map<std::string, TokenType> keywords = {
        { "лента",          TokenType::TAPE       },
        { "пока",           TokenType::WHILE      },
        { "конец",          TokenType::END        },
        { "есть_бит",       TokenType::HAS_BIT    },
        { "инвертировать",  TokenType::CMD_INVERT },
        { "сложить",        TokenType::CMD_ADD    },
};

Lexer::Lexer(const std::string& source)
        : source_(source), pos_(0), line_(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();

        if (c == ':') {
            advance();
            tokens.push_back({ TokenType::COLON, "", line_ });

            //после "лента:" сразу читаем строку данных
            skipWhitespace();
            if (!isAtEnd() && peek() != '\n') {
                tokens.push_back(readString());
            }
            continue;
        }

        if (c == '#') {
            //комментарий скипаем до конца строки
            while (!isAtEnd() && peek() != '\n') advance();
            continue;
        }

        //слово
        if (c != ':' && c != '#' && c != '\n' && c != '\r' && c != ' ' && c != '\t') {
            tokens.push_back(readWord());
            continue;
        }

        //неизвестный символ тогда ошибка с номером строки
        throw std::runtime_error(
                "Неизвестный символ '" + std::string(1, c) +
                "' на строке " + std::to_string(line_)
        );
    }

    tokens.push_back({ TokenType::END_OF_FILE, "", line_ });
    return tokens;
}


void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == '\n') { line_++; advance(); }
        else if (c == ' ' || c == '\t' || c == '\r') advance();
        else break;
    }
}

Token Lexer::readWord() {
    std::string word;
    int startLine = line_;

    while (!isAtEnd()) {
        char c = peek();
        if (c != ':' && c != '#' && c != '\n' && c != '\r' && c != ' ' && c != '\t') {
            word += advance();
        } else {
            break;
        }
    }

    //ищем в таблице ключевых слов
    auto it = keywords.find(word);
    if (it != keywords.end()) {
        return { it->second, "", startLine };
    }

    //неизвестное слово - ошибка
    throw std::runtime_error(
            "Неизвестное слово '" + word +
            "' на строке " + std::to_string(startLine)
    );
}

Token Lexer::readString() {
    std::string value;
    int startLine = line_;

    while (!isAtEnd() && peek() != '\n') {
        value += advance();
    }

    //убираем пробелы по краям чтобы
    auto start = value.find_first_not_of(" \t");
    auto end   = value.find_last_not_of(" \t\r");
    if (start != std::string::npos)
        value = value.substr(start, end - start + 1);

    return { TokenType::STRING, value, startLine };
}

char Lexer::peek() const {
    return source_[pos_];
}

char Lexer::advance() {
    return source_[pos_++];
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}