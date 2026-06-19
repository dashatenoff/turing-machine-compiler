#include "gui_render.h"
#include "raylib.h"

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>

static Font gFont;

static const int CELL_W  = 44;
static const int CELL_H  = 44;
static const int VISIBLE = 12;

static const Color BG          = { 30,  30,  46,  255 };
static const Color PANEL       = { 45,  45,  68,  255 };
static const Color CELL_NORMAL = { 60,  60,  90,  255 };
static const Color CELL_HEAD   = { 231, 76,  60,  255 };
static const Color CELL_DATA   = { 52,  152, 219, 255 };
static const Color TEXT_MAIN   = { 236, 240, 241, 255 };
static const Color TEXT_DIM    = { 127, 140, 141, 255 };
static const Color BTN_GREEN   = { 39,  174, 96,  255 };
static const Color BTN_BLUE    = { 41,  128, 185, 255 };
static const Color BTN_ORANGE  = { 230, 126, 34,  255 };
static const Color BTN_GRAY    = { 100, 100, 120, 255 };
static const Color BTN_RED     = { 192, 57,  43,  255 };

enum class Screen { EDITOR, RUNNING };
static Screen gScreen       = Screen::EDITOR;
static int    gSpeed        = 5;
static int    gEditorScroll = 0;
static bool gExamplesOpen = false;
// позиция курсора в байтах в строке input.programText
static int    gCursorPos    = 0;

// сколько кадров прошло с запуска — чтобы игнорировать phantom backspace
static int    gFrameCount   = 0;

static void txt(const char* text, int x, int y, int size, Color col) {
    DrawTextEx(gFont, text, { (float)x, (float)y }, (float)size, 1.0f, col);
}

static int measure(const char* text, int size) {
    return (int)MeasureTextEx(gFont, text, (float)size, 1.0f).x;
}

static bool btn(int x, int y, int w, int h, const char* text, Color col) {
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    Color c = hover ? Color{
            (unsigned char)std::min(col.r + 30, 255),
            (unsigned char)std::min(col.g + 30, 255),
            (unsigned char)std::min(col.b + 30, 255), 255 } : col;
    DrawRectangleRec(r, c);
    DrawRectangleLinesEx(r, 1, { 200, 200, 200, 60 });
    int tw = measure(text, 16);
    txt(text, x + (w - tw) / 2, y + (h - 16) / 2, 16, TEXT_MAIN);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void drawTape(const TapeSnap& snap, int x, int y, const char* label) {
    txt(label, x, y - 22, 14, TEXT_DIM);
    int total = VISIBLE * 2 + 1;
    for (int i = 0; i < total; i++) {
        long pos     = snap.head + (i - VISIBLE);
        auto it      = snap.cells.find(pos);
        char ch      = (it != snap.cells.end()) ? it->second : '_';
        bool isHead  = (i == VISIBLE);
        bool hasData = (it != snap.cells.end() && ch != '_');
        Color fill   = isHead ? CELL_HEAD : (hasData ? CELL_DATA : CELL_NORMAL);
        Color border = isHead ? Color{255,100,80,255} : Color{80,80,120,255};
        int cx = x + i * (CELL_W + 2);
        DrawRectangle(cx, y, CELL_W, CELL_H, fill);
        DrawRectangleLines(cx, y, CELL_W, CELL_H, border);
        char tmp[2] = { ch, '\0' };
        int tw = measure(tmp, 20);
        txt(tmp, cx + (CELL_W - tw)/2, y + (CELL_H - 20)/2, 20,
            isHead ? WHITE : TEXT_MAIN);
        if (pos % 5 == 0) {
            std::string ps = std::to_string(pos);
            int pw = measure(ps.c_str(), 9);
            txt(ps.c_str(), cx + (CELL_W - pw)/2, y + CELL_H + 3, 9, TEXT_DIM);
        }
    }
    int hx = x + VISIBLE * (CELL_W + 2) + CELL_W / 2;
    DrawTriangle(
            { (float)hx,       (float)(y - 2)  },
            { (float)(hx - 7), (float)(y - 12) },
            { (float)(hx + 7), (float)(y - 12) },
            CELL_HEAD
    );
    std::string hp = "голова: " + std::to_string(snap.head);
    txt(hp.c_str(), x, y + CELL_H + 16, 11, TEXT_DIM);
}

static std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : text) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else cur += c;
    }
    lines.push_back(cur);
    return lines;
}

// длина utf8 символа по первому байту
static int utf8CharLen(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    return 1;
}

// вставляет utf8 codepoint в строку на позиции pos возвращает новую позицию
static int insertCodepoint(std::string& s, int pos, int cp) {
    std::string enc;
    if (cp < 0x80) {
        enc += (char)cp;
    } else if (cp < 0x800) {
        enc += (char)(0xC0 | (cp >> 6));
        enc += (char)(0x80 | (cp & 0x3F));
    } else {
        enc += (char)(0xE0 | (cp >> 12));
        enc += (char)(0x80 | ((cp >> 6) & 0x3F));
        enc += (char)(0x80 | (cp & 0x3F));
    }
    s.insert(pos, enc);
    return pos + (int)enc.size();
}

// удаляет utf8 символ перед позицией pos возвращает новую позицию
static int deleteBeforeCursor(std::string& s, int pos) {
    if (pos <= 0) return 0;
    int end = pos;
    pos--;
    while (pos > 0 && (s[pos] & 0xC0) == 0x80) pos--;
    s.erase(pos, end - pos);
    return pos;
}

// возвращает байтовую позицию в тексте по клику мыши в редакторе
static int posFromClick(
        const std::string& text,
        int mouseX, int mouseY,
        int edX, int edY, int lineH, int scroll, int fontSize)
{
    std::vector<std::string> lines = splitLines(text);
    int clickedLine = (mouseY - edY) / lineH + scroll;
    clickedLine = std::max(0, std::min(clickedLine, (int)lines.size() - 1));

    // находим байтовое начало строки clickedLine в тексте
    int byteOffset = 0;
    for (int i = 0; i < clickedLine; i++) {
        byteOffset += (int)lines[i].size() + 1; // +1 за \n
    }

    // внутри строки ищем ближайший символ по x
    const std::string& line = lines[clickedLine];
    int bestPos = 0;
    int bestDist = 99999;
    int byteIdx = 0;
    int x = edX;
    // проверяем позицию перед каждым символом
    while (true) {
        int dist = std::abs(mouseX - x);
        if (dist < bestDist) { bestDist = dist; bestPos = byteOffset + byteIdx; }
        if (byteIdx >= (int)line.size()) break;
        // ширина одного utf8 символа
        int clen = utf8CharLen((unsigned char)line[byteIdx]);
        std::string ch = line.substr(byteIdx, clen);
        x += measure(ch.c_str(), fontSize);
        byteIdx += clen;
    }
    return bestPos;
}

bool guiInit(int w, int h) {
    InitWindow(w, h, "Машина Тьюринга");
    SetTargetFPS(60);

    int codepointCount = 0;
    int codepoints[300];
    for (int i = 32; i < 127; i++)
        codepoints[codepointCount++] = i;
    for (int i = 0x0410; i <= 0x044F; i++)
        codepoints[codepointCount++] = i;
    codepoints[codepointCount++] = 0x0401;
    codepoints[codepointCount++] = 0x0451;

    gFont = LoadFontEx("C:/Windows/Fonts/arial.ttf", 20, codepoints, codepointCount);
    SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
    return true;
}

void guiClose() {
    UnloadFont(gFont);
    CloseWindow();
}

bool guiShouldClose() {
    return WindowShouldClose();
}

GuiEvent guiFrame(GuiState& state, GuiInput& input) {
    gFrameCount++;
    GuiEvent ev = GuiEvent::NONE;
    int SW = GetScreenWidth();
    int SH = GetScreenHeight();

    static const int ED_X      = 58;
    static const int ED_NUM_X  = 28;
    static const int ED_Y      = 88;
    static const int ED_LINE_H = 17;
    static const int ED_MAX_Y  = 262;
    static const int ED_VISIBLE = (ED_MAX_Y - ED_Y) / ED_LINE_H;
    static const int ED_FONT   = 13;

    // клампим курсор
    gCursorPos = std::max(0, std::min(gCursorPos, (int)input.programText.size()));

    if (gScreen == Screen::EDITOR) {

        // символы
        int key = GetCharPressed();
        while (key > 0) {
            if (input.programText.size() < 2000) {
                gCursorPos = insertCodepoint(input.programText, gCursorPos, key);
            }
            key = GetCharPressed();
        }

        // Enter
        if (IsKeyPressed(KEY_ENTER)) {
            input.programText.insert(gCursorPos, "\n");
            gCursorPos++;
        }

        // Backspace — игнорируем первые 5 кадров чтобы убрать phantom
        if (gFrameCount > 5 && IsKeyPressed(KEY_BACKSPACE)) {
            gCursorPos = deleteBeforeCursor(input.programText, gCursorPos);
        }

        // стрелки влево/вправо
        if (IsKeyPressed(KEY_LEFT) && gCursorPos > 0) {
            gCursorPos--;
            while (gCursorPos > 0 &&
                   (input.programText[gCursorPos] & 0xC0) == 0x80)
                gCursorPos--;
        }
        if (IsKeyPressed(KEY_RIGHT) &&
            gCursorPos < (int)input.programText.size()) {
            int clen = utf8CharLen(
                    (unsigned char)input.programText[gCursorPos]);
            gCursorPos += clen;
        }

        // скролл колесом
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
            gEditorScroll -= (int)wheel;

        // клик мышью — позиция курсора
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            if (mp.x >= 20 && mp.x <= SW - 20 &&
                mp.y >= ED_Y && mp.y < ED_MAX_Y) {
                gCursorPos = posFromClick(
                        input.programText,
                        (int)mp.x, (int)mp.y,
                        ED_X, ED_Y, ED_LINE_H,
                        gEditorScroll, ED_FONT);
            }
        }
    }

    BeginDrawing();
    ClearBackground(BG);

    if (gScreen == Screen::EDITOR) {

        txt("Машина Тьюринга", 20, 12, 22, TEXT_MAIN);
        txt("компилятор РУССКОГО языка", 20, 38, 13, TEXT_DIM);

        DrawRectangle(20, 65, SW - 40, 205, PANEL);
        DrawRectangleLines(20, 65, SW - 40, 205, {80,80,120,255});
        txt("Программа (.mt):", ED_NUM_X, 70, 12, TEXT_DIM);

        std::vector<std::string> lines = splitLines(input.programText);
        int totalLines = (int)lines.size();

        // находим строку и колонку курсора
        int cursorLine = 0, cursorByte = 0;
        {
            int byteCount = 0;
            for (int i = 0; i < (int)lines.size(); i++) {
                int lineEnd = byteCount + (int)lines[i].size();
                if (gCursorPos <= lineEnd) {
                    cursorLine = i;
                    cursorByte = gCursorPos - byteCount;
                    break;
                }
                byteCount += (int)lines[i].size() + 1;
                cursorLine = i;
                cursorByte = (int)lines[i].size();
            }
        }

        // автоскролл к курсору
        if (cursorLine - gEditorScroll >= ED_VISIBLE)
            gEditorScroll = cursorLine - ED_VISIBLE + 1;
        if (cursorLine < gEditorScroll)
            gEditorScroll = cursorLine;
        if (gEditorScroll < 0) gEditorScroll = 0;
        if (gEditorScroll > std::max(0, totalLines - 1))
            gEditorScroll = std::max(0, totalLines - 1);

        BeginScissorMode(20, ED_Y - 2, SW - 40, ED_MAX_Y - ED_Y + 4);
        for (int i = gEditorScroll; i < totalLines; i++) {
            int ly = ED_Y + (i - gEditorScroll) * ED_LINE_H;
            if (ly >= ED_MAX_Y) break;

            // подсветка текущей строки
            if (i == cursorLine)
                DrawRectangle(20, ly - 1, SW - 42, ED_LINE_H,
                              {70, 70, 100, 255});

            // номер строки
            txt(std::to_string(i + 1).c_str(), ED_NUM_X, ly, 12, TEXT_DIM);

            // текст строки
            txt(lines[i].c_str(), ED_X, ly, ED_FONT, TEXT_MAIN);

            // курсор
            if (i == cursorLine && (int)(GetTime() * 2) % 2 == 0) {
                std::string before = lines[i].substr(0, cursorByte);
                int cx = ED_X + measure(before.c_str(), ED_FONT);
                DrawRectangle(cx, ly, 2, ED_LINE_H - 2, TEXT_MAIN);
            }
        }
        EndScissorMode();

        // скроллбар
        if (totalLines > ED_VISIBLE) {
            int sbX = SW - 32;
            int sbH = ED_MAX_Y - ED_Y;
            DrawRectangle(sbX, ED_Y, 6, sbH, {60,60,90,255});
            int thumbH = std::max(16, sbH * ED_VISIBLE / totalLines);
            int maxSc  = std::max(1, totalLines - ED_VISIBLE);
            int thumbY = ED_Y + (sbH - thumbH) * gEditorScroll / maxSc;
            DrawRectangle(sbX, thumbY, 6, thumbH, BTN_GRAY);
        }

        // кнопки
        if (btn(20, 278, 220, 36, "Компилировать", BTN_GREEN))
            ev = GuiEvent::COMPILE;

        // кнопка "Примеры" с выпадающим списком из экзаймплс
        {
            bool openedNow = btn(250, 278, 130, 36, "Примеры =)", BTN_GRAY);
            if (openedNow) gExamplesOpen = !gExamplesOpen;

            if (gExamplesOpen) {
                int dropX = 250;
                int dropY = 318;
                int dropW = 260;
                int rowH  = 28;
                int count = (int)input.exampleNames.size();
                int dropH = count > 0 ? count * rowH + 8 : 36;

                DrawRectangle(dropX, dropY, dropW, dropH, PANEL);
                DrawRectangleLines(dropX, dropY, dropW, dropH, {80,80,120,255});

                if (count == 0) {
                    txt("нет файлов .mt", dropX + 10, dropY + 10, 13, TEXT_DIM);
                } else {
                    for (int i = 0; i < count; i++) {
                        int ry = dropY + 4 + i * rowH;
                        Rectangle r = { (float)dropX, (float)ry, (float)dropW, (float)rowH };
                        bool hover = CheckCollisionPointRec(GetMousePosition(), r);
                        if (hover) DrawRectangleRec(r, {70, 70, 100, 255});

                        txt(input.exampleNames[i].c_str(), dropX + 10, ry + 6, 14, TEXT_MAIN);

                        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            input.selectedExample = i;
                            ev = GuiEvent::LOAD_EXAMPLE;
                            gExamplesOpen = false;
                        }
                    }
                }

                // клик мимо списка — закрыть
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                    !CheckCollisionPointRec(GetMousePosition(),
                                            { (float)dropX, (float)dropY, (float)dropW, (float)dropH }) &&
                    !openedNow) {
                    gExamplesOpen = false;
                }
            }
        }
        txt("Синтаксис:", 20, 328, 12, TEXT_DIM);
        txt("лента: <данные>", 20, 344, 12, TEXT_MAIN);
        txt("инвертировать", 20, 360, 12, TEXT_MAIN);
        txt("сложить   (A+B, например 101+110)", 20, 376, 12, TEXT_MAIN);
        txt("пока есть_бит:", 20, 392, 12, TEXT_MAIN);
        txt("  инвертировать", 20, 408, 12, TEXT_MAIN);
        txt("конец", 20, 424, 12, TEXT_MAIN);

        int logTop = 448;
        DrawRectangle(20, logTop, SW - 40, SH - logTop - 28, PANEL);
        txt("Лог:", 28, logTop + 4, 11, TEXT_DIM);
        int logY  = logTop + 18;
        int start = std::max(0, (int)state.logLines.size() - 9);
        for (int i = start;
             i < (int)state.logLines.size() && logY < SH - 32; i++) {
            txt(state.logLines[i].c_str(), 28, logY, 11, TEXT_MAIN);
            logY += 15;
        }

        txt(state.statusMsg.c_str(), 20, SH - 20, 11, TEXT_DIM);

    } else { // RUNNING

        txt("Визуализация", 20, 8, 18, TEXT_MAIN);

        std::string stStr = state.halted ? "HALT" : state.currentState;
        txt("Состояние:", 20, 32, 12, TEXT_DIM);
        txt(stStr.c_str(), 115, 32, 13, state.halted ? BTN_RED : BTN_GREEN);
        std::string stps = "Шагов: " + std::to_string(state.steps);
        txt(stps.c_str(), 350, 32, 13, TEXT_DIM);
        if (!state.result.empty()) {
            std::string rs = "Результат: " + state.result;
            txt(rs.c_str(), 600, 32, 13, BTN_GREEN);
        }

        drawTape(state.tape1, 20, 80,  "Лента 1  (входная)");
        drawTape(state.tape2, 20, 195, "Лента 2  (выходная)");

        DrawLine(0, 280, SW, 280, {80,80,120,180});

        int bx = 20, by = 288, bh = 36;
        if (btn(bx,     by, 90,  bh, "Шаг",        BTN_BLUE))   ev = GuiEvent::STEP;
        if (btn(bx+100, by, 90,  bh,
                state.halted ? "Готово" : "Пуск",   BTN_GREEN))  ev = GuiEvent::RUN;
        if (btn(bx+200, by, 90,  bh, "Пауза",      BTN_ORANGE)) ev = GuiEvent::PAUSE;
        if (btn(bx+300, by, 90,  bh, "Сброс",      BTN_GRAY))   ev = GuiEvent::RESET;
        if (btn(bx+400, by, 140, bh, "< Редактор", BTN_GRAY)) {
            gScreen = Screen::EDITOR;
            ev = GuiEvent::BACK_TO_EDITOR;
        }

        txt("Скорость:", bx+555, by+10, 12, TEXT_DIM);
        if (btn(bx+640, by+4, 26, 26, "-", BTN_GRAY)) {
            gSpeed = std::max(1, gSpeed - 1);
            ev = GuiEvent::SPEED_DOWN;
        }
        txt(std::to_string(gSpeed).c_str(), bx+672, by+10, 14, TEXT_MAIN);
        if (btn(bx+690, by+4, 26, 26, "+", BTN_GRAY)) {
            gSpeed = std::min(10, gSpeed + 1);
            ev = GuiEvent::SPEED_UP;
        }

        DrawRectangle(20, 335, SW - 40, 190, PANEL);
        txt("Лог:", 28, 340, 11, TEXT_DIM);
        int logY  = 355;
        int start = std::max(0, (int)state.logLines.size() - 11);
        for (int i = start;
             i < (int)state.logLines.size() && logY < 520; i++) {
            txt(state.logLines[i].c_str(), 28, logY, 11, TEXT_MAIN);
            logY += 15;
        }

        DrawLine(0, 530, SW, 530, {80,80,120,180});

        std::string t1raw = "Лента 1: ";
        for (auto& [p,c] : state.tape1.cells) if (c != '_') t1raw += c;
        std::string t2raw = "Лента 2: ";
        for (auto& [p,c] : state.tape2.cells) if (c != '_') t2raw += c;
        txt(t1raw.c_str(), 20, 537, 11, TEXT_DIM);
        txt(t2raw.c_str(), 20, 552, 11,
            state.halted ? BTN_GREEN : TEXT_DIM);

        txt(state.statusMsg.c_str(), 20, SH - 20, 11, TEXT_DIM);
    }

    EndDrawing();
    return ev;
}

void guiSwitchToRunning() { gScreen = Screen::RUNNING; }
void guiSwitchToEditor()  { gScreen = Screen::EDITOR;  }
int  guiGetSpeed()        { return gSpeed; }