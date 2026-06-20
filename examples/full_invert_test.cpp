#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#include <iostream>

int main()
{

    std::string source =
            "лента: 1011\n"
            "инвертировать\n";

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    std::unique_ptr<ProgramNode> ast = parser.parse();

    Compiler compiler;
    TMProgram program = compiler.compile(*ast);

    TuringMachine tm(program);

    tm.run();

    std::cout << "Tape2: "
              << tm.getTape2().toString()
              << std::endl;

    return 0;
}