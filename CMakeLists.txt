# Двухленточная машина Тьюринга + компилятор мини-языка
cmake_minimum_required(VERSION 3.10)
project(lab5)

set(CMAKE_CXX_STANDARD 17)

# Заголовки лежат в папке include/, эта строчка говорит компилятору
# искать их там — чтобы #include "tape.h" находился без полного пути.
include_directories(include)

# Общие исходники: ядро машины Тьюринга + компилятор мини-языка.
# Собрала их в одну переменную COMMON, чтобы не перечислять один и тот же
# список в каждой цели. Заголовки тоже добавляю — тогда они видны в дереве
# проекта в IDE.
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

# Консольная версия: общие файлы + своя точка входа cli_main.cpp
add_executable(tm-cli
        src/cli_main.cpp
        ${COMMON}
)

# --- Тесты ---------------------------------------------------------
# Каждый тест — отдельная маленькая программа со своим main.
# Собираются вместе с проектом; запускать вручную: ./test_tape и т.д.
# (в CLion появятся как отдельные цели рядом с tm-cli).
add_executable(test_tape            examples/test_tape.cpp            ${COMMON})
add_executable(turing_machine_test  examples/turing_machine_test.cpp  ${COMMON})
add_executable(full_invert_test     examples/full_invert_test.cpp     ${COMMON})
add_executable(compiler_invert_test examples/compiler_invert_test.cpp ${COMMON})
add_executable(macros_test          examples/macros_test.cpp          ${COMMON})

# compiler_add_test пока отключён: написан под старый API, и сложение
# (emitAdd) ещё не доделано. Раскомментируем, когда починим.
# add_executable(compiler_add_test examples/compiler_add_test.cpp ${COMMON})
