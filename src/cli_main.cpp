#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#ifdef _WIN32
#include <windows.h>   // только для Windows, настройка кодовой страницы консоли
#endif

//читает весь файл в строку
static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {

#ifdef _WIN32
    // Говорим консоли Windows, что вывод в UTF-8 — иначе русский текст
    // показывается кракозябрами (╨У╨╛╤В╨╛╨▓╨╛ вместо «Готово»).
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Использование: tm-cli <файл.mt>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        std::unique_ptr<ProgramNode> ast = parser.parse();

        Compiler compiler;
        TMProgram program = compiler.compile(*ast);
        TuringMachine tm(program);
        tm.run();
        std::cout << "Готово. Шагов: " << tm.getsteps() << std::endl;
        std::cout << "Tape2: "
                  << tm.getTape2().toString()
                  << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}