#include "parser.h"

Parser::Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}//парсер забирает себе владение вектором токенов


std::unique_ptr<ProgramNode> Parser::parse() {
    return parseProgram();
}

//  программа = объявление ленты + список команд
std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto node = std::make_unique<ProgramNode>();
    expect(TokenType::TAPE,  "начало программы");
    expect(TokenType::COLON, "объявление ленты");
    const Token& str = expect(TokenType::STRING, "значение ленты");
    node->tapeValue = str.value;
    node->line      = str.line;

    //тело программы остальное
    while (!isAtEnd() && !check(TokenType::END_OF_FILE)) {
        node->body.push_back(parseStatement());
    }

    return node;
}
NodePtr Parser::parseStatement() { //просто выбирет че вызвать
    const Token& t = peek();

    if (t.type == TokenType::WHILE)      return parseLoop();
    if (t.type == TokenType::CMD_INVERT) return parseCommand();
    if (t.type == TokenType::CMD_ADD)    return parseCommand();
    throw std::runtime_error(
            "Ожидалась команда или 'пока', "
            "но найдено '" + t.value + "' на строке " +
            std::to_string(t.line)
    );
}

std::unique_ptr<LoopNode> Parser::parseLoop() {
    auto node = std::make_unique<LoopNode>();
    node->line = peek().line;

    expect(TokenType::WHILE, "цикл пока");

    if (check(TokenType::HAS_BIT)) {
        advance();
        node->condition = ConditionType::HAS_BIT;
    } else {
        throw std::runtime_error(
                "Неизвестное условие цикла на строке " +
                std::to_string(peek().line) +
                ". Ожидалось: есть_бит"
        );
    }

    expect(TokenType::COLON, "после условия цикла");

    //тело цикла до 'конец'
    while (!isAtEnd() && !check(TokenType::END)) {
        node->body.push_back(parseStatement());
    }

    expect(TokenType::END, "конец цикла пока");

    return node;
}

//одна команда
std::unique_ptr<CommandNode> Parser::parseCommand() {
    auto node = std::make_unique<CommandNode>();
    const Token& t = advance();//expect используется когда мы ожидаем конкретный токен и хотим
    // автоматическую проверку с стандартным сообщением об ошибке голый advance()
    // используется когда проверка типа уже была сделана раньше (как в parseCommand)
    // либо когда нужно особое сообщение об ошибке
    node->line = t.line;

    if (t.type == TokenType::CMD_INVERT) {
        node->command = CommandType::INVERT;
    } else if (t.type == TokenType::CMD_ADD) {
        node->command = CommandType::ADD;
    }

    return node;
}


const Token& Parser::peek() const {
    return tokens_[pos_];
}

const Token& Parser::advance() {
    if (!isAtEnd()) pos_++;
    return tokens_[pos_ - 1];//возвращаем тот токен что съели
}

bool Parser::check(TokenType type) const {
    return tokens_[pos_].type == type;
}

bool Parser::isAtEnd() const {
    return tokens_[pos_].type == TokenType::END_OF_FILE;
}

const Token& Parser::expect(TokenType type, const std::string& where) {//если тек токен нужного типа съедаем
    if (check(type)) return advance();

    throw std::runtime_error(
            "Ошибка в " + where +
            " на строке " + std::to_string(peek().line) +
            ": неожиданный токен '" + peek().value + "'"
    );
}