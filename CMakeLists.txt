cmake_minimum_required(VERSION 3.10)
project(lab5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE
            "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

set(COMMON
        src/tape.cpp
        src/turing_machine.cpp
        src/lexer.cpp
        src/parser.cpp
        src/compiler.cpp
)

include_directories(include)

add_executable(tm-cli src/cli_main.cpp ${COMMON})

add_executable(test_tape            examples/test_tape.cpp            ${COMMON})
add_executable(turing_machine_test  examples/turing_machine_test.cpp  ${COMMON})
add_executable(full_invert_test     examples/full_invert_test.cpp     ${COMMON})
add_executable(compiler_invert_test examples/compiler_invert_test.cpp ${COMMON})
add_executable(compiler_add_test    examples/compiler_add_test.cpp    ${COMMON})

# ── GUI (ImGui + SDL2 + OpenGL) ──────────────────────────────────────────────

add_library(imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl2.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PUBLIC SDL2::SDL2 opengl32)

find_package(SDL2 REQUIRED)

add_executable(tm-gui
        src/gui_main.cpp
        src/gui_render.cpp
        ${COMMON}
)
target_include_directories(tm-gui PRIVATE include imgui imgui/backends)
target_link_libraries(tm-gui PRIVATE imgui SDL2::SDL2 opengl32)