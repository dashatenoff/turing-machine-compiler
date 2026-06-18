#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#include <iostream>

int main()
{
    // программа на нашем мини-языке прямо в тесте —
    // так тест не зависит от того, из какой папки его запустили
    std::string source =
            "лента: 1011\n"
            "инвертировать\n";

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