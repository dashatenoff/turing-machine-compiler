#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#include <iostream>
#include <fstream>

int main()
{
    std::ifstream file("examples/add.mt");

    Lexer lexer(file);
    Parser parser(lexer);

    ProgramNode ast = parser.parse();

    Compiler compiler;
    TMProgram program = compiler.compile(ast);

    TuringMachine tm(program);

    tm.run();

    std::cout << "Tape2: "
              << tm.getTape2().toString()
              << std::endl;
}