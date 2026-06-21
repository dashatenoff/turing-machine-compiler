#include "compiler.h"
#include <stdexcept>

// вспомогательный метод добавляет одно правило чтобы не копировать код и не ошибиться
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

    // обходим тело каждый узел получает entry возвращает exit а exit становится следующим entry
    std::string last = compileBody(program.body, entry);

    // последнее состояние -> HALT stay на обеих лентах тогда головки не двигаются
    addRule(last, '_', '_', '_', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '_', '0', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '_', '1', '_', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '0', '0', '0', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '0', '1', '0', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '0', '1', '0', '1', Move::Stay, Move::Stay, StateGenerator::halt());
    addRule(last, '1', '1', '1', '1', Move::Stay, Move::Stay, StateGenerator::halt());

    return { StateGenerator::start(), rules_, tapeValue_ };
}

// обход списка узлов возвращает финальное состояние после блока
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

// вызывает нужный макрос
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

    // entry -> cond просто переход stay на обоих лентах
    addRule(entry, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    addRule(entry, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    addRule(entry, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    addRule(entry, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    // cond если '_' на ленте 1 условие ложно выходим
    if (node.condition == ConditionType::HAS_BIT) {
        addRule(cond, '_', '_', '_', '_', Move::Stay, Move::Stay, exit);
        addRule(cond, '_', '0', '_', '0', Move::Stay, Move::Stay, exit);
        addRule(cond, '_', '1', '_', '1', Move::Stay, Move::Stay, exit);
    }

    // cond если не '_' условие истинно идём в тело
    std::string bodyEntry = gen_.next("while_body");
    addRule(cond, '0', '_', '0', '_', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '_', '1', '_', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '0', '0', '0', '0', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '0', '1', '0', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '0', '1', '0', '1', Move::Stay, Move::Stay, bodyEntry);
    addRule(cond, '1', '1', '1', '1', Move::Stay, Move::Stay, bodyEntry);

    // компилируем тело последнее состояние тела должно вернуться обратно в cond
    std::string bodyExit = compileBody(node.body, bodyEntry);

    // bodyExit -> cond замыкаем цикл
    addRule(bodyExit, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    addRule(bodyExit, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    return exit;
}

// emitInvert
// лента1: 1 0 1 1 _       лента2: _ _ _ _ _
// едем вправо по ленте1
// каждый бит инвертируем и пишем на ленту2
// '_' на ленте1 конец переходим в exit
void Compiler::emitInvert(
        const std::string& entry,
        const std::string& exit)
{
    // '0' -> пишем '1' на ленту2 двигаемся вправо
    addRule(entry, '0', '_', '0', '1', Move::Right, Move::Right, entry);
    // '1' -> пишем '0' на ленту2 двигаемся вправо
    addRule(entry, '1', '_', '1', '0', Move::Right, Move::Right, entry);
    // '_' на ленте1 конец данных
    addRule(entry, '_', '_', '_', '_', Move::Stay, Move::Stay, exit);
}

// emitAdd
// лента1: A+B например "101+110"
// лента2 на выходе содержит сумму A+B в двоичном виде

void Compiler::emitAdd(
        const std::string& entry,
        const std::string& exit)
{
    // фаза 1 пропускаем A ищем '+'
    std::string skip_a = gen_.next("add_skip_a");
    addRule(entry,  '0', '_', '0', '_', Move::Right, Move::Stay, skip_a);
    addRule(entry,  '1', '_', '1', '_', Move::Right, Move::Stay, skip_a);
    addRule(skip_a, '0', '_', '0', '_', Move::Right, Move::Stay, skip_a);
    addRule(skip_a, '1', '_', '1', '_', Move::Right, Move::Stay, skip_a);

    // нашли '+' начинаем копировать B
    std::string copy_b = gen_.next("add_copy_b");
    addRule(skip_a, '+', '_', '+', '_', Move::Right, Move::Stay, copy_b);
    addRule(copy_b, '0', '_', '0', '0', Move::Right, Move::Right, copy_b);
    addRule(copy_b, '1', '_', '1', '1', Move::Right, Move::Right, copy_b);

    // конец B теперь лента1 на '_' после B лента2 на '_' после скопированного B
    // фаза 2 rewind1: только лента1 влево лента2 stay
    std::string rw1 = gen_.next("add_rewind1");
    addRule(copy_b, '_', '_', '_', '_', Move::Left, Move::Stay, rw1);

    // лента2 стоит на '_' следующий шаг влево 
    std::string sb = gen_.next("add_step_back");

    // едем влево по ленте1 до '+'
    addRule(rw1, '0', '_', '0', '_', Move::Left, Move::Stay, rw1);
    addRule(rw1, '1', '_', '1', '_', Move::Left, Move::Stay, rw1);
    addRule(rw1, '+', '_', '+', '_', Move::Stay, Move::Stay, sb);

    // add_nc без переноса add_wc с переносом
    std::string add_nc = gen_.next("add_nc");
    std::string add_wc = gen_.next("add_wc");


    addRule(sb, '+', '_', '+', '_', Move::Left, Move::Left, add_nc);

    // add_nc без переноса
    // 0+0=0
    addRule(add_nc, '0', '0', '0', '0', Move::Left, Move::Left, add_nc);
    // 0+1=1
    addRule(add_nc, '0', '1', '0', '1', Move::Left, Move::Left, add_nc);
    // 1+0=1
    addRule(add_nc, '1', '0', '1', '1', Move::Left, Move::Left, add_nc);
    // 1+1=0 перенос
    addRule(add_nc, '1', '1', '1', '0', Move::Left, Move::Left, add_wc);

    // лента1 вышла за пределы A (читаем '_') но лента2 ещё есть данные
    addRule(add_nc, '_', '0', '_', '0', Move::Stay, Move::Stay, exit);
    addRule(add_nc, '_', '1', '_', '1', Move::Stay, Move::Stay, exit);
    addRule(add_nc, '_', '_', '_', '_', Move::Stay, Move::Stay, exit);

    // лента2 вышла за пределы B но лента1 ещё есть данные
    addRule(add_nc, '0', '_', '0', '0', Move::Left, Move::Left, add_nc);
    addRule(add_nc, '1', '_', '1', '1', Move::Left, Move::Left, add_nc);

    // add_wc с переносом
    // 0+0+1=1 нет переноса
    addRule(add_wc, '0', '0', '0', '1', Move::Left, Move::Left, add_nc);
    // 0+1+1=0 перенос
    addRule(add_wc, '0', '1', '0', '0', Move::Left, Move::Left, add_wc);
    // 1+0+1=0 перенос
    addRule(add_wc, '1', '0', '1', '0', Move::Left, Move::Left, add_wc);
    // 1+1+1=1 перенос
    addRule(add_wc, '1', '1', '1', '1', Move::Left, Move::Left, add_wc);

    // конец A при add_wc остался перенос
    // если лента2 тоже кончилась дописываем '1' на ленту2
    addRule(add_wc, '_', '_', '_', '1', Move::Stay, Move::Stay, exit);
    // лента2 ещё есть данные продолжаем с переносом только по ленте2
    addRule(add_wc, '_', '0', '_', '1', Move::Stay, Move::Left, add_nc);
    addRule(add_wc, '_', '1', '_', '0', Move::Stay, Move::Left, add_wc);

    // лента2 кончилась при add_wc но лента1 ещё есть дописываем перенос к биту A
    addRule(add_wc, '0', '_', '0', '1', Move::Left, Move::Left, add_nc);
    addRule(add_wc, '1', '_', '1', '0', Move::Left, Move::Left, add_wc);
}