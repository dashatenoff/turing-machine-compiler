# Двухленточная машина Тьюринга + компилятор мини-языка
cmake_minimum_required(VERSION 3.10)
project(lab5)

set(CMAKE_CXX_STANDARD 17)



include_directories(include)


set(COMMON
        src/tape.cpp
        src/turing_machine.cpp
        src/lexer.cpp
        src/parser.cpp
        src/compiler.cpp
        src/macros.cpp
        src/tm_program.cpp
        src/tm_format.cpp

        include/tape.h
        include/turing_machine.h
        include/rule.h
        include/token.h
        include/lexer.h
        include/ast.h
        include/parser.h
        include/compiler.h
        include/macros.h
        include/state_generator.h
        include/tm_program.h
        include/tm_format.h
)


add_executable(tm-cli
        src/cli_main.cpp
        ${COMMON}
)


add_executable(test_tape            examples/test_tape.cpp            ${COMMON})
add_executable(turing_machine_test  examples/turing_machine_test.cpp  ${COMMON})
add_executable(full_invert_test     examples/full_invert_test.cpp     ${COMMON})
add_executable(compiler_invert_test examples/compiler_invert_test.cpp ${COMMON})
add_executable(macros_test          examples/macros_test.cpp          ${COMMON})


# add_executable(compiler_add_test examples/compiler_add_test.cpp ${COMMON})
