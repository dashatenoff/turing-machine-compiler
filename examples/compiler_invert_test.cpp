#include "compiler.h"
#include "turing_machine.h"
#include "ast.h"

#include <iostream>
#include <memory>

int main()
{
    ProgramNode ast;

    ast.tapeValue = "1011";

    auto cmd = std::make_unique<CommandNode>();
    cmd->command = CommandType::INVERT;

    ast.body.push_back(std::move(cmd));

    Compiler compiler;

    TMProgram program = compiler.compile(ast);

    TuringMachine tm(program);

    tm.run();

    std::cout << "Tape2: "
              << tm.getTape2().toString()
              << std::endl;

    return 0;
}