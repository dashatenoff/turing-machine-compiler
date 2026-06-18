# Двухленточная машина Тьюринга + компилятор мини-языка
cmake_minimum_required(VERSION 3.10)
project(lab5)

set(CMAKE_CXX_STANDARD 17)

# Заголовки лежат в папке include/, эта строчка говорит компилятору
# искать их там — чтобы #include "tape.h" находился без полного пути.
include_directories(include)

# Общие исходники: ядро машины Тьюринга + компилятор мини-языка.
# Их используют ОБЕ версии программы (и консольная, и графическая),
# поэтому я собрала их в одну переменную COMMON, чтобы не перечислять
# один и тот же список дважды. Заголовки тоже добавляю — тогда они
# видны в дереве проекта в IDE.
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

# Графическая версия (на SFML).
# find_package ищет установленную в системе библиотеку SFML.
# QUIET — не ругаться, если её нет; тогда SFML_FOUND будет ложным
# и графическую версию мы просто не собираем, а консольная работает как обычно.
find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
if (SFML_FOUND)
    add_executable(tm-gui
            src/gui_main.cpp
            ${COMMON}
    )
    target_link_libraries(tm-gui sfml-graphics sfml-window sfml-system)
endif ()
