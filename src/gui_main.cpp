#include "gui_render.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "turing_machine.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

static TapeSnap makeSnap(const Tape& t) {
    return { t.getCells(), t.getPosition() };
}

static void refreshState(GuiState& gs, TuringMachine* tm, bool halted) {
    if (!tm) return;
    gs.tape1        = makeSnap(tm->getTape1());
    gs.tape2        = makeSnap(tm->getTape2());
    gs.currentState = tm->getCurrentState();
    gs.steps        = tm->getsteps();
    gs.halted       = halted;
    if (halted) gs.result = tm->getTape2().toString();
}


//  Ищет examples/ рядом с рабочей директорией.
static fs::path findExamplesDir() {
    std::vector<fs::path> candidates = {
            "examples",
            "../examples",
            "../../examples",
            fs::current_path() / "examples"
    };
    for (const auto& p : candidates) {
        if (fs::exists(p) && fs::is_directory(p)) {
            return p;
        }
    }
    return fs::path(); // — пустой путь
}


//  Сканирует папку examples, собирает все .mt файлы.
//  Возвращает пары, имя_без_расширения - полный_путь
static std::vector<std::pair<std::string, fs::path>> scanExamples() {
    std::vector<std::pair<std::string, fs::path>> result;
    fs::path dir = findExamplesDir();
    if (dir.empty()) return result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mt") {
            result.push_back({ entry.path().stem().string(), entry.path() });
        }
    }
    return result;
}

//прочитать файл целиком в одну строку
static std::string readFileToString(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    guiInit(1100, 700);

    GuiState gs; // Хранит то, что показывается пользователю
    GuiInput gi; // Хранит то, что пользователь вводит
    gi.programText = "лента: 101+110\nсложить";
    gs.statusMsg   = "Введите программу и нажмите Компилировать";

    //Автозагрузка списка примеров
    auto examples = scanExamples();
    for (const auto& [name, path] : examples) {
        gi.exampleNames.push_back(name);
    }
    if (examples.empty()) {
        gs.logLines.push_back("Папка examples/ не найдена или пуста");
    } else {
        gs.logLines.push_back("Найдено примеров: " + std::to_string(examples.size()));
    }

    TMProgram program;
    std::unique_ptr<TuringMachine> tm;
    bool compiled = false;
    bool running  = false;
    bool halted   = false;

    using clock = std::chrono::steady_clock;
    auto lastStep = clock::now();

    while (!guiShouldClose()) {
        // автопрогон
        if (running && tm && !halted) {
            float delay = 1.1f - guiGetSpeed() * 0.1f;
            auto now = clock::now();
            float elapsed = std::chrono::duration<float>(now - lastStep).count();
            if (elapsed >= delay) {
                lastStep = now;
                bool ok = tm->step();
                refreshState(gs, tm.get(), false);
                if (!ok || tm->halted()) {
                    halted  = true;
                    running = false;
                    refreshState(gs, tm.get(), true);
                    gs.logLines.push_back("HALT. Шагов: " + std::to_string(tm->getsteps()));
                    std::string r = tm->getTape2().toString();
                    gs.logLines.push_back("Лента 2: " + (r.empty() ? "(пусто)" : r));
                    gs.statusMsg = "Завершено. Результат: " + (r.empty() ? "(пусто)" : r);
                }
            }
        }

        GuiEvent ev = guiFrame(gs, gi);

        switch (ev) {

            //Загрузка примера по клику 
            case GuiEvent::LOAD_EXAMPLE: {
                if (gi.selectedExample >= 0 &&
                    gi.selectedExample < (int)examples.size()) {
                    const auto& [name, path] = examples[gi.selectedExample];
                    std::string content = readFileToString(path);
                    if (!content.empty()) {
                        gi.programText = content;
                        gs.statusMsg   = "Загружен пример: " + name;
                    } else {
                        gs.statusMsg = "Не удалось прочитать: " + name;
                    }
                }
                gi.selectedExample = -1;
                break;
            }

            case GuiEvent::COMPILE: {
                compiled = false; halted = false; running = false;
                gs.logLines.clear(); gs.result = "";
                tm.reset();
                try {
                    Lexer lexer(gi.programText);
                    auto tokens = lexer.tokenize();
                    Parser parser(std::move(tokens));
                    auto ast = parser.parse();
                    Compiler compiler;
                    program  = compiler.compile(*ast);
                    tm       = std::make_unique<TuringMachine>(program);
                    compiled = true;
                    refreshState(gs, tm.get(), false);
                    gs.logLines.push_back("Компиляция OK. Правил: " +
                                          std::to_string(program.rules.size()));
                    gs.logLines.push_back("Лента 1: " + program.tapeValue);
                    gs.statusMsg = "Скомпилировано";
                    guiSwitchToRunning();
                } catch (const std::exception& e) {
                    gs.logLines.push_back("ОШИБКА: " + std::string(e.what()));
                    gs.statusMsg = "Ошибка компиляции";
                }
                break;
            }

            case GuiEvent::STEP:
                if (tm && !halted && !running) {
                    bool ok = tm->step();
                    refreshState(gs, tm.get(), false);
                    if (!ok || tm->halted()) {
                        halted = true;
                        refreshState(gs, tm.get(), true);
                        std::string r = tm->getTape2().toString();
                        gs.logLines.push_back("HALT. Шагов: " + std::to_string(tm->getsteps()));
                        gs.logLines.push_back("Лента 2: " + (r.empty() ? "(пусто)" : r));
                        gs.statusMsg = "Завершено. Результат: " + (r.empty() ? "(пусто)" : r);
                    }
                }
                break;

            case GuiEvent::RUN:
                if (tm && !halted) { running = true; lastStep = clock::now(); }
                break;

            case GuiEvent::PAUSE:
                running = false;
                gs.statusMsg = "Пауза";
                break;

            case GuiEvent::RESET:
                if (compiled) {
                    tm      = std::make_unique<TuringMachine>(program);
                    halted  = false;
                    running = false;
                    gs.result = "";
                    gs.logLines.clear();
                    refreshState(gs, tm.get(), false);
                    gs.logLines.push_back("Сброс. Лента 1: " + program.tapeValue);
                    gs.statusMsg = "Сброшено";
                }
                break;

            case GuiEvent::BACK_TO_EDITOR:
                running = false;
                break;

            default: break;
        }
    }

    guiClose();
    return 0;
}