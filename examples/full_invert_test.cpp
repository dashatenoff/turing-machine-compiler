#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#include <fstream>
#include <iostream>
#include <sstream>

int main()
{
    // читаем весь файл в строку
    std::ifstream file("examples/invert.mt");

    if (!file.is_open()) {
        std::cout << "Cannot open invert.mt" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();

    // Lexer
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // Parser
    Parser parser(tokens);
    std::unique_ptr<ProgramNode> ast = parser.parse();

    // Compiler
    Compiler compiler;
    TMProgram program = compiler.compile(*ast);

    // Machine
    TuringMachine tm(program);

    tm.run();

    std::cout << "Tape2: "
              << tm.getTape2().toString()
              << std::endl;

    return 0;
}