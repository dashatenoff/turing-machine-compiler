#pragma once
#include <string>
#include <map>
#include <vector>

struct TapeSnap {
    std::map<long, char> cells;
    long head;
};

struct GuiState {
    TapeSnap tape1;
    TapeSnap tape2;
    std::string currentState;
    long steps;
    bool halted;
    std::string result;
    std::vector<std::string> logLines;
    std::string statusMsg;
};

enum class GuiEvent {
    NONE,
    COMPILE,
    STEP,
    RUN,
    PAUSE,
    RESET,
    BACK_TO_EDITOR,
    SPEED_UP,
    SPEED_DOWN,
    LOAD_EXAMPLE
};

struct GuiInput {
    std::string programText;
    std::vector<std::string> exampleNames;  //имена файлов без .mt для отрисовки списка
    int         selectedExample = -1;       //индекс примера который нажали читает main
};

bool  guiInit(int w, int h);
void  guiClose();
bool  guiShouldClose();
GuiEvent guiFrame(GuiState& state, GuiInput& input);

void guiSwitchToRunning();
void guiSwitchToEditor();
int  guiGetSpeed();