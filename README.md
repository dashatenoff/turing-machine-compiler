# Turing Machine Compiler
# Машина Тьюринга — компилятор и симулятор

Двухленточная машина Тьюринга с собственным языком программирования на русском языке.

## Структура проекта

```
include/      — заголовочные файлы (.h)
src/          — исходный код (.cpp)
examples/     — примеры программ (.mt), автоматически подгружаются в GUI
raylib/       — графическая библиотека (ставится отдельно)
vcpkg/        — менеджер C++ пакетов (ставится отдельно)
ARCHITECTURE.md — архитектура проекта
```

## Установка на новом компьютере

### 1. Склонировать репозиторий

```
git clone https://github.com/dashatenoff/turing-machine-compiler.git
cd turing-machine-compiler
```

### 2. Поставить vcpkg

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
cd ..
```

### 3. Поставить raylib через vcpkg

```
.\vcpkg\vcpkg install raylib
```

### 4. Открыть проект в CLion

1. File → Open → выбрать папку `turing-machine-compiler`
2. CLion сам найдёт `CMakeLists.txt`
3. Settings → Build, Execution, Deployment → CMake → в поле **CMake options** добавить:

```
-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
```

4. Нажать **Reload CMake Project**

### 5. Собрать и запустить

