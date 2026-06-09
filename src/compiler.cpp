#include "compiler.h"
#include <stdexcept>

//вспомогательный метод добавляет одно правило чтобы не копировать код и не ошибиться
void Compiler::addRule(
        const std::string& cur,
        char r1, char r2,
        char w1, char w2,
        Move m1, Move m2,
        const std::string& next)
{
    rules_.push_back({ cur, r1, r2, w1, w2, m1, m2, next });
}


TMProgram Compiler::compile(const ProgramNode& program) {
    rules_.clear();
    tapeValue_ = program.tapeValue;

    std::string entry = StateGenerator::start();

    //обходим тело каждый узел получает entry возвращает exit а exit становится следующим entry
    std::string last = compileBody(program.body, entry);

    //последнее состояние -> HALT
    //Stay на обеих лентах тогда головки не двигаются
    addRule(last, '_', '_', '_', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '_', '0', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '_', '1', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '0', '0', '0', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '0', '1', '0', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '1', '0', '1', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '1', '1', '1', Move::Stay, Move::Stay, StateGenerator::halt());

    return { StateGenerator::start(), rules_, tapeValue_ };
}

//обход списка узлов возвращает финальное состояние после блока
std::string Compiler::compileBody(
        const std::vector<NodePtr>& body,
        const std::string& entry)
{
    std::string cur = entry;
    for (const auto& node : body) {
        cur = compileStatement(*node, cur);
    }
    return cur;
}

std::string Compiler::compileStatement(
        const Node& node,
        const std::string& entry)
{
    if (const auto* cmd = dynamic_cast<const CommandNode*>(&node)) {
        return compileCommand(*cmd, entry);
    }
    if (const auto* loop = dynamic_cast<const LoopNode*>(&node)) {
        return compileLoop(*loop, entry);
    }
    throw std::runtime_error(
            "Compiler: неизвестный тип узла на строке " +
            std::to_string(node.line)
    );
}
//вызывает нужный макрос
std::string Compiler::compileCommand(
        const CommandNode& node,
        const std::string& entry)
{
    std::string exit = gen_.next(
            node.command == CommandType::INVERT ? "invert" : "add"
    );

    switch (node.command) {
        case CommandType::INVERT:
            emitInvert(entry, exit);
            break;
        case CommandType::ADD:
            emitAdd(entry, exit);
            break;
        default:
            throw std::runtime_error(
                    "Compiler: неизвестная команда на строке " +
                    std::to_string(node.line)
            );
    }
    return exit;
}

std::string Compiler::compileLoop(
        const LoopNode& node,
        const std::string& entry)
{
    std::string cond = gen_.next("while_cond");
    std::string exit = gen_.next("while_exit");

    //entry -> cond (просто переход, Stay на обоих лентах)
    addRule(entry, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    addRule(entry, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    //сond: если '_' на ленте 1 — условие ложно, выходим
    if (node.condition == ConditionType::HAS_BIT) {
        addRule(cond, '_', '_', '_', '_', Move::Stay, Move::Stay, exit);
        addRule(cond, '_', '0', '_', '0', Move::Stay, Move::Stay, exit);
        addRule(cond, '_', '1', '_', '1', Move::Stay, Move::Stay, exit);
    }

    //cond: если не '_' — условие истинно, идём в тело
    std::string bodyEntry = gen_.next("while_body");
    addRule(cond, '0', '_', '0', '_', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '_', '1', '_', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '0', '0', '0', '0', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '0', '1', '0', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '0', '1', '0', '1', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '1', '1', '1', Move::Stay, Move::Stay, bodyEntry);

    // Компилируем тело это последнее состояние тела должно вернуться обратно в cond
    std::string bodyExit = compileBody(node.body, bodyEntry);

    //bodyExit -> cond (замыкаем цикл)
    addRule(bodyExit, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    return exit;
}

// ─────────────────────────────────────────────
//  emitInvert
//
//  Лента 1: 1 0 1 1 _       Лента 2: _ _ _ _ _
//  Едем вправо по ленте 1.
//  Каждый бит инвертируем и пишем на ленту 2.
//  '_' на ленте 1 — конец, переходим в exit.
// ─────────────────────────────────────────────
void Compiler::emitInvert(
        const std::string& entry,
        const std::string& exit)
{
    // '0' -> пишем '1' на ленту 2, двигаемся вправо
    addRule(entry, '0', '_', '0', '1', Move::Right, Move::Right, entry);
    // '1' -> пишем '0' на ленту 2, двигаемся вправо
    addRule(entry, '1', '_', '1', '0', Move::Right, Move::Right, entry);
    // '_' на ленте 1 — конец данных
    addRule(entry, '_', '_', '_', '_', Move::Stay,  Move::Stay,  exit);
}

//  emitAdd
//  Лента 1: 1 0 1 + 1 1 0 _
//  Алгоритм:
//  Фаза 1 (scan_plus):
//    едем вправо по ленте 1 пока не найдём '+'
//  Фаза 2 (copy_b):
//    едем вправо, копируем правый операнд на ленту 2
//    до '_' — конец правого операнда
//  Фаза 3 (rewind):
//    возвращаем обе головки влево до начала
//  Фаза 4 (add_nc / add_wc):
//    едем вправо по обеим лентам
//    складываем попарно биты с переносом
//    результат пишем на ленту 2
void Compiler::emitAdd(
        const std::string& entry,
        const std::string& exit)
{
    std::string scan_plus = gen_.next("add_scan_plus");

    addRule(entry, '0', '_', '0', '_', Move::Right, Move::Stay, scan_plus);
    addRule(entry, '1', '_', '1', '_', Move::Right, Move::Stay, scan_plus);
    addRule(scan_plus, '0', '_', '0', '_', Move::Right, Move::Stay, scan_plus);
    addRule(scan_plus, '1', '_', '1', '_', Move::Right, Move::Stay, scan_plus);

    std::string copy_b = gen_.next("add_copy_b");
    addRule(scan_plus, '+', '_', '+', '_', Move::Right, Move::Stay, copy_b);
    addRule(copy_b, '0', '_', '0', '0', Move::Right, Move::Right, copy_b);
    addRule(copy_b, '1', '_', '1', '1', Move::Right, Move::Right, copy_b);
    std::string rewind = gen_.next("add_rewind");
    addRule(copy_b, '_', '_', '_', '_', Move::Left, Move::Left, rewind);

    addRule(rewind, '0', '0', '0', '0', Move::Left, Move::Left, rewind);
    addRule(rewind, '1', '1', '1', '1', Move::Left, Move::Left, rewind);
    addRule(rewind, '0', '1', '0', '1', Move::Left, Move::Left, rewind);
    addRule(rewind, '1', '0', '1', '0', Move::Left, Move::Left, rewind);
    addRule(rewind, '+', '0', '+', '0', Move::Left, Move::Left, rewind);
    addRule(rewind, '+', '1', '+', '1', Move::Left, Move::Left, rewind);

    std::string add_nc = gen_.next("add_nc"); //no carry
    addRule(rewind, '_', '_', '_', '_', Move::Right, Move::Right, add_nc);
    std::string add_wc   = gen_.next("add_wc");   //with carry
    std::string write_c  = gen_.next("add_write_carry"); //дописать перенос
    addRule(add_nc, '0', '0', '0', '0', Move::Right, Move::Right, add_nc);
    //0 + 1 = 1, нет переноса
    addRule(add_nc, '0', '1', '0', '1', Move::Right, Move::Right, add_nc);
    addRule(add_nc, '1', '0', '1', '1', Move::Right, Move::Right, add_nc);
    //1 + 1 = 0, есть перенос
    addRule(add_nc, '1', '1', '1', '0', Move::Right, Move::Right, add_wc);

    //конец одного из операндов при add_nc если лента 1 кончилась то остаток ленты 2 как есть
    addRule(add_nc, '+', '0', '+', '0', Move::Right, Move::Stay, add_nc);
    addRule(add_nc, '+', '1', '+', '1', Move::Right, Move::Stay, add_nc);
    addRule(add_nc, '_', '_', '_', '_', Move::Stay,  Move::Stay,  exit);
    addRule(add_nc, '_', '0', '_', '0', Move::Stay,  Move::Stay,  exit);
    addRule(add_nc, '_', '1', '_', '1', Move::Stay,  Move::Stay,  exit);

    //add_wc складываем с переносом
    addRule(add_wc, '0', '0', '0', '1', Move::Right, Move::Right, add_nc);
    addRule(add_wc, '0', '1', '0', '0', Move::Right, Move::Right, add_wc);
    // 1 + 0 + 1 = 0 есть перенос
    addRule(add_wc, '1', '0', '1', '0', Move::Right, Move::Right, add_wc);
    addRule(add_wc, '1', '1', '1', '1', Move::Right, Move::Right, add_wc);

    //конец данных при add_wc остался перенос тогда дописываем '1'
    addRule(add_wc, '_', '_', '_', '1', Move::Stay,  Move::Right, exit);
    addRule(add_wc, '+', '0', '+', '0', Move::Right, Move::Stay,  add_wc);
    addRule(add_wc, '+', '1', '+', '1', Move::Right, Move::Stay,  add_wc);
    addRule(add_wc, '_', '0', '_', '1', Move::Stay,  Move::Stay,  exit);
    addRule(add_wc, '_', '1', '_', '0', Move::Stay,  Move::Right, add_wc);
}