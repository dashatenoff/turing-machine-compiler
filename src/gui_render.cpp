// gui_render.cpp  —  ImGui + SDL2 + OpenGL3  (замена raylib)
#include "gui_render.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>

// ─── цвета (те же что в оригинале) ───────────────────────────────────────────
static ImVec4 c(int r,int g,int b,int a=255){
    return {r/255.f,g/255.f,b/255.f,a/255.f};
}
static const ImVec4 BG          = c(30,  30,  46);
static const ImVec4 PANEL       = c(45,  45,  68);
static const ImVec4 CELL_NORMAL = c(60,  60,  90);
static const ImVec4 CELL_HEAD   = c(231, 76,  60);
static const ImVec4 CELL_DATA   = c(52,  152, 219);
static const ImVec4 TEXT_MAIN   = c(236, 240, 241);
static const ImVec4 TEXT_DIM    = c(127, 140, 141);
static const ImVec4 BTN_GREEN   = c(39,  174, 96);
static const ImVec4 BTN_BLUE    = c(41,  128, 185);
static const ImVec4 BTN_ORANGE  = c(230, 126, 34);
static const ImVec4 BTN_GRAY    = c(100, 100, 120);
static const ImVec4 BTN_RED     = c(192, 57,  43);

static ImU32 toU32(ImVec4 v){
    return IM_COL32(
            (int)(v.x*255),(int)(v.y*255),
            (int)(v.z*255),(int)(v.w*255));
}

// ─── внутреннее состояние ────────────────────────────────────────────────────
static SDL_Window*   s_window  = nullptr;
static SDL_GLContext s_glctx   = nullptr;
static bool          s_running = false;

enum class Screen { EDITOR, RUNNING };
static Screen s_screen       = Screen::EDITOR;
static int    s_speed        = 5;

// буфер редактора
static char   s_textBuf[1 << 16] = {};
static bool   s_textBufDirty     = true; // нужно ли синхронизировать из gi

// ─── helpers ─────────────────────────────────────────────────────────────────

// кнопка с нужным цветом
static bool colorBtn(const char* label, ImVec4 col, ImVec2 size = {0,0}) {
    ImGui::PushStyleColor(ImGuiCol_Button,        col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
            std::min(col.x+0.12f,1.f),
            std::min(col.y+0.12f,1.f),
            std::min(col.z+0.12f,1.f), 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(
            std::max(col.x-0.1f,0.f),
            std::max(col.y-0.1f,0.f),
            std::max(col.z-0.1f,0.f), 1.f));
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return r;
}

static void textCol(const char* s, ImVec4 col) {
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(s);
    ImGui::PopStyleColor();
}

// ─── отрисовка одной ленты ────────────────────────────────────────────────────
static void drawTape(const char* label, const TapeSnap& snap) {
    const int VISIBLE = 12;
    const float CELL_W = 44.f;
    const float CELL_H = 44.f;

    textCol(label, TEXT_DIM);

    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2      pos = ImGui::GetCursorScreenPos();

    // треугольник над головкой (центр ячейки головки)
    float headScreenX = pos.x + VISIBLE * (CELL_W + 2.f) + CELL_W * 0.5f;
    float triY        = pos.y - 2.f;
    dl->AddTriangleFilled(
            {headScreenX,      triY},
            {headScreenX - 7,  triY - 10},
            {headScreenX + 7,  triY - 10},
            toU32(CELL_HEAD));

    int total = VISIBLE * 2 + 1;
    for (int i = 0; i < total; i++) {
        long cellPos = snap.head + (i - VISIBLE);
        auto it      = snap.cells.find(cellPos);
        char ch      = (it != snap.cells.end()) ? it->second : '_';
        bool isHead  = (i == VISIBLE);
        bool hasData = (it != snap.cells.end() && ch != '_');

        ImVec4 fillV  = isHead ? CELL_HEAD : (hasData ? CELL_DATA : CELL_NORMAL);
        ImU32  fill   = toU32(fillV);
        ImU32  border = isHead
                        ? IM_COL32(255,100,80,255)
                        : IM_COL32(80,80,120,255);

        float cx = pos.x + i * (CELL_W + 2.f);
        float cy = pos.y;

        dl->AddRectFilled({cx,cy},{cx+CELL_W,cy+CELL_H}, fill, 3.f);
        dl->AddRect      ({cx,cy},{cx+CELL_W,cy+CELL_H}, border, 3.f);

        // символ по центру ячейки
        char tmp[2] = {ch, '\0'};
        ImVec2 tsz  = ImGui::CalcTextSize(tmp);
        ImU32  tcol = isHead
                      ? IM_COL32(255,255,255,255)
                      : toU32(TEXT_MAIN);
        dl->AddText(
                {cx + (CELL_W - tsz.x)*0.5f, cy + (CELL_H - tsz.y)*0.5f},
                tcol, tmp);

        // номер позиции каждые 5
        if (cellPos % 5 == 0) {
            std::string ns = std::to_string(cellPos);
            ImVec2 nsz = ImGui::CalcTextSize(ns.c_str());
            dl->AddText(
                    {cx + (CELL_W - nsz.x)*0.5f, cy + CELL_H + 3.f},
                    toU32(TEXT_DIM), ns.c_str());
        }
    }

    // двигаем курсор ImGui вниз на высоту ленты + подписи + стрелки
    ImGui::Dummy({total*(CELL_W+2.f), CELL_H + 22.f});

    // подпись "голова: N"
    std::string hp = "голова: " + std::to_string(snap.head);
    textCol(hp.c_str(), TEXT_DIM);
}

// ─── init / close ────────────────────────────────────────────────────────────
bool guiInit(int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    s_window = SDL_CreateWindow(
            "Машина Тьюринга",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            w, h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window) { SDL_Quit(); return false; }

    s_glctx = SDL_GL_CreateContext(s_window);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // шрифт с кириллицей — arial лежит на любом Windows
    static const ImWchar ranges[] = {
            0x0020, 0x00FF,
            0x0400, 0x04FF,
            0,
    };
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    ImFont* fnt = io.Fonts->AddFontFromFileTTF(
            "C:/Windows/Fonts/arial.ttf", 15.f, &cfg, ranges);
    if (!fnt) io.Fonts->AddFontDefault();

    // стиль: тёмный, как оригинал
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding    = 0.f;
    st.FrameRounding     = 3.f;
    st.ScrollbarRounding = 3.f;
    st.WindowBorderSize  = 0.f;
    st.Colors[ImGuiCol_WindowBg]  = BG;
    st.Colors[ImGuiCol_ChildBg]   = PANEL;
    st.Colors[ImGuiCol_FrameBg]   = c(40,40,62);
    st.Colors[ImGuiCol_ScrollbarBg]   = c(40,40,62);
    st.Colors[ImGuiCol_ScrollbarGrab] = c(100,100,120);
    st.Colors[ImGuiCol_Text]      = TEXT_MAIN;

    ImGui_ImplSDL2_InitForOpenGL(s_window, s_glctx);
    ImGui_ImplOpenGL3_Init("#version 330");

    s_running = true;
    return true;
}

void guiClose() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(s_glctx);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}

bool guiShouldClose() { return !s_running; }
void guiSwitchToRunning() { s_screen = Screen::RUNNING; }
void guiSwitchToEditor()  { s_screen = Screen::EDITOR;  }
int  guiGetSpeed()        { return s_speed; }

// ─── главный кадр ────────────────────────────────────────────────────────────
GuiEvent guiFrame(GuiState& gs, GuiInput& gi) {
    GuiEvent ev = GuiEvent::NONE;

    // SDL события
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT) s_running = false;
    }

    // синхронизируем буфер если снаружи изменился programText (загрузка примера)
    {
        static std::string lastKnown;
        if (gi.programText != lastKnown) {
            lastKnown = gi.programText;
            strncpy(s_textBuf, gi.programText.c_str(), sizeof(s_textBuf)-1);
            s_textBuf[sizeof(s_textBuf)-1] = '\0';
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    int ww, wh;
    SDL_GetWindowSize(s_window, &ww, &wh);

    // полноэкранное окно без рамки
    ImGui::SetNextWindowPos({0,0});
    ImGui::SetNextWindowSize({(float)ww,(float)wh});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 10.f});
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    // ══════════════════════════════════════════════════════════════════════════
    if (s_screen == Screen::EDITOR) {
        // ══════════════════════════════════════════════════════════════════════════

        // заголовок
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        ImGui::SetWindowFontScale(1.4f);
        ImGui::Text("Машина Тьюринга");
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("компилятор РУССКОГО языка");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // панель редактора
        ImGui::PushStyleColor(ImGuiCol_ChildBg, PANEL);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Программа (.mt):");
        ImGui::PopStyleColor();

        // поле ввода — высота ~12 строк как в оригинале
        float editorH = 205.f;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, c(40,40,62));
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        if (ImGui::InputTextMultiline(
                "##prog", s_textBuf, sizeof(s_textBuf),
                {(float)ww - 32.f, editorH},
                ImGuiInputTextFlags_AllowTabInput)) {
            gi.programText = s_textBuf;
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleColor(); // ChildBg

        ImGui::Spacing();

        // кнопка Компилировать
        if (colorBtn("Компилировать", BTN_GREEN, {220, 36}))
            ev = GuiEvent::COMPILE;

        ImGui::SameLine();

        // кнопка Примеры + выпадающий список.
        // Через штатный popup ImGui (OpenPopup/BeginPopup): он сам корректно
        // держит фокус и z-order и закрывается по клику мимо. Раньше список
        // рисовался отдельным окном поверх полноэкранного ##root, и клики по
        // нему перехватывал ##root — пример не вставлялся.
        bool examplesBtn = colorBtn("Примеры =)", BTN_GRAY, {130, 36});
        ImVec2 exMin = ImGui::GetItemRectMin();
        ImVec2 exMax = ImGui::GetItemRectMax();
        if (examplesBtn)
            ImGui::OpenPopup("##examples_popup");

        // прижимаем popup под кнопку
        ImGui::SetNextWindowPos({exMin.x, exMax.y + 2.f});
        ImGui::PushStyleColor(ImGuiCol_PopupBg, PANEL);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        if (ImGui::BeginPopup("##examples_popup")) {
            if (gi.exampleNames.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
                ImGui::TextUnformatted("нет файлов .mt");
                ImGui::PopStyleColor();
            } else {
                for (int i = 0; i < (int)gi.exampleNames.size(); i++) {
                    if (ImGui::Selectable(gi.exampleNames[i].c_str())) {
                        gi.selectedExample = i;
                        ev = GuiEvent::LOAD_EXAMPLE;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // подсказка синтаксиса
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Синтаксис:");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        ImGui::Text("лента: <данные>");
        ImGui::Text("инвертировать");
        ImGui::Text("сложить   (A+B, например 101+110)");
        ImGui::Text("пока есть_бит:");
        ImGui::Text("  инвертировать");
        ImGui::Text("конец");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // лог
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Лог:");
        ImGui::PopStyleColor();

        float logH = (float)wh - ImGui::GetCursorPosY() - 30.f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, PANEL);
        ImGui::BeginChild("##log", {0, logH}, true);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        int start = std::max(0, (int)gs.logLines.size() - 9);
        for (int i = start; i < (int)gs.logLines.size(); i++)
            ImGui::TextUnformatted(gs.logLines[i].c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // статус внизу
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("%s", gs.statusMsg.c_str());
        ImGui::PopStyleColor();

        // ══════════════════════════════════════════════════════════════════════════
    } else { // RUNNING
        // ══════════════════════════════════════════════════════════════════════════

        // заголовок
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("Визуализация");
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor();

        ImGui::SameLine(200);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Состояние:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              gs.halted ? BTN_RED : BTN_GREEN);
        ImGui::Text("%s", gs.halted ? "HALT" : gs.currentState.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(500);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Шагов: %ld", gs.steps);
        ImGui::PopStyleColor();

        if (!gs.result.empty()) {
            ImGui::SameLine(700);
            ImGui::PushStyleColor(ImGuiCol_Text, BTN_GREEN);
            ImGui::Text("Результат: %s", gs.result.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // ленты
        drawTape("Лента 1  (входная)",  gs.tape1);
        ImGui::Spacing();
        drawTape("Лента 2  (выходная)", gs.tape2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // кнопки управления
        if (colorBtn("Шаг",    BTN_BLUE,   {90,36})) ev = GuiEvent::STEP;
        ImGui::SameLine();
        if (colorBtn(gs.halted ? "Готово" : "Пуск", BTN_GREEN, {90,36}))
            ev = GuiEvent::RUN;
        ImGui::SameLine();
        if (colorBtn("Пауза",  BTN_ORANGE, {90,36})) ev = GuiEvent::PAUSE;
        ImGui::SameLine();
        if (colorBtn("Сброс",  BTN_GRAY,   {90,36})) ev = GuiEvent::RESET;
        ImGui::SameLine();
        if (colorBtn("< Редактор", BTN_GRAY, {140,36})) {
            s_screen = Screen::EDITOR;
            ev = GuiEvent::BACK_TO_EDITOR;
        }

        // скорость кнопками + / -  (как в оригинале)
        ImGui::SameLine(700);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Скорость:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (colorBtn("-##spd", BTN_GRAY, {26,26})) {
            s_speed = std::max(1, s_speed - 1);
            ev = GuiEvent::SPEED_DOWN;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        ImGui::Text("%d", s_speed);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (colorBtn("+##spd", BTN_GRAY, {26,26})) {
            s_speed = std::min(10, s_speed + 1);
            ev = GuiEvent::SPEED_UP;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // лог
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("Лог:");
        ImGui::PopStyleColor();

        float logH = (float)wh - ImGui::GetCursorPosY() - 60.f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, PANEL);
        ImGui::BeginChild("##log2", {0, logH}, true);
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_MAIN);
        int start = std::max(0, (int)gs.logLines.size() - 11);
        for (int i = start; i < (int)gs.logLines.size(); i++)
            ImGui::TextUnformatted(gs.logLines[i].c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.f);
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Separator();

        // сырые данные лент внизу
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        std::string t1raw = "Лента 1: ";
        for (auto& [p,ch] : gs.tape1.cells) if (ch != '_') t1raw += ch;
        ImGui::Text("%s", t1raw.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text,
                              gs.halted ? BTN_GREEN : TEXT_DIM);
        std::string t2raw = "Лента 2: ";
        for (auto& [p,ch] : gs.tape2.cells) if (ch != '_') t2raw += ch;
        ImGui::Text("%s", t2raw.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM);
        ImGui::Text("%s", gs.statusMsg.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::End();

    // рендер
    ImGui::Render();
    glViewport(0, 0, ww, wh);
    glClearColor(BG.x, BG.y, BG.z, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(s_window);

    return ev;
}