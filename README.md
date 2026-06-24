# Turing Machine Compiler

Двухленточная машина Тьюринга с собственным языком программирования на русском языке.

## Структура проекта

    include/        — заголовочные файлы (.h)
    src/            — исходный код (.cpp)
    examples/       — примеры программ (.mt), автоматически подгружаются в GUI
    imgui/          — графическая библиотека (ставится отдельно, см. ниже)
    vcpkg/          — менеджер C++ пакетов (ставится отдельно)
    ARCHITECTURE.md — архитектура проекта

## Установка на новом компьютере

### 1. Склонировать репозиторий

    git clone https://github.com/dashatenoff/turing-machine-compiler.git
    cd turing-machine-compiler

### 2. Поставить vcpkg

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
    cd ..

### 3. Поставить SDL2 через vcpkg

    .\vcpkg\vcpkg install sdl2:x64-windows

### 4. Скачать Dear ImGui

    git clone https://github.com/ocornut/imgui.git --depth=1

Папка imgui/ должна лежать в корне проекта рядом с CMakeLists.txt

### 5. Открыть проект в CLion

1. File → Open → выбрать папку проекта
2. CLion сам найдёт CMakeLists.txt
3. Settings → Build, Execution, Deployment → CMake → в поле CMake options добавить:

   -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake

4. Нажать Reload CMake Project

### 6. Собрать и запустить

- Выбрать цель tm-gui и нажать Run
- Для CLI: выбрать цель tm-cli и передать .mt файл как аргумент

## Зависимости

| Библиотека | Способ установки                      |
|------------|---------------------------------------|
| SDL2       | vcpkg install sdl2:x64-windows        |
| Dear ImGui | git clone (папка imgui/ в корне)      |
| OpenGL     | встроен в Windows (opengl32)          |

GUI работает на встроенной видеокарте (Intel и т.п.) — использует OpenGL 3.3 Core через SDL2.