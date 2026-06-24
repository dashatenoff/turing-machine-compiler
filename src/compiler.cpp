#include "compiler.h"
#include <stdexcept>

//прост вспомогательный метод добавляет одно правило чтобы не копировать код и не ошибиться
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

    // compileBody обходит узлы а каждый узел сам знает как себя скомпилировать
    std::string last = compileBody(program.body, entry);

    //соединяем последнее состояние с HALT
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
    for (const NodePtr& node : body) {
        cur = node->compile(*this, cur);
    }
    return cur;
}

//реализации виртуальных методов

std::string ProgramNode::compile(Compiler& compiler, const std::string& entry) const {
    //ProgramNode компилирует своё тело
    return compiler.compileBody(body, entry);
}

std::string CommandNode::compile(Compiler& compiler, const std::string& entry) const {
    std::string exit = compiler.gen_.next(
            command == CommandType::INVERT ? "invert" : "add"
    );

    switch (command) {
        case CommandType::INVERT:
            compiler.emitInvert(entry, exit);
            break;
        case CommandType::ADD:
            compiler.emitAdd(entry, exit);
            break;
        default:
            throw std::runtime_error(
                    "неизвестная команда на строке " + std::to_string(line)
            );
    }
    return exit;
}

std::string LoopNode::compile(Compiler& compiler, const std::string& entry) const {
    std::string cond      = compiler.gen_.next("while_cond");
    std::string exit      = compiler.gen_.next("while_exit");

    //entry -> cond переход stay на обоих лентах чтобы работатть уже с состоянием проверки условия цикла
    //комбинации '_' на ленте 1 с чем угодно на ленте 2 мы не включаем сюда тк они обрабатываются в блоке cond
    compiler.addRule(entry, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    compiler.addRule(entry, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    // cond если '_' на ленте1 условие ложно выходим
    if (condition == ConditionType::HAS_BIT) {
        compiler.addRule(cond, '_', '_', '_', '_', Move::Stay, Move::Stay, exit);
        compiler.addRule(cond, '_', '0', '_', '0', Move::Stay, Move::Stay, exit);
        compiler.addRule(cond, '_', '1', '_', '1', Move::Stay, Move::Stay, exit);
    }

    // cond если не '_' условие истинно идём в тело
    //bodyEntry новое уникальное имя для точки входа в тело цикла
    std::string bodyEntry = compiler.gen_.next("while_body");
    compiler.addRule(cond, '0', '_', '0', '_', Move::Stay, Move::Stay, bodyEntry);
    compiler.addRule(cond, '1', '_', '1', '_', Move::Stay, Move::Stay, bodyEntry);
    compiler.addRule(cond, '0', '0', '0', '0', Move::Stay, Move::Stay, bodyEntry);
    compiler.addRule(cond, '1', '0', '1', '0', Move::Stay, Move::Stay, bodyEntry);
    compiler.addRule(cond, '0', '1', '0', '1', Move::Stay, Move::Stay, bodyEntry);
    compiler.addRule(cond, '1', '1', '1', '1', Move::Stay, Move::Stay, bodyEntry);

    // компилируем тело рекурсивно
    std::string bodyExit = compiler.compileBody(body, bodyEntry);

    //bodyExit -> cond замыкаем цикл
    compiler.addRule(bodyExit, '0', '_', '0', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '1', '_', '1', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '_', '_', '_', '_', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '0', '0', '0', '0', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '1', '0', '1', '0', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '0', '1', '0', '1', Move::Stay, Move::Stay, cond);
    compiler.addRule(bodyExit, '1', '1', '1', '1', Move::Stay, Move::Stay, cond);

    return exit;
}

// едем вправо по ленте1
// каждый бит инвертируем и пишем на ленту2
void Compiler::emitInvert(
        const std::string& entry,
        const std::string& exit)
{
    // '0' пишем '1' на ленту2 двигаемся вправо
    addRule(entry, '0', '_', '0', '1', Move::Right, Move::Right, entry);
    // '1'  пишем '0' на ленту2 двигаемся вправо
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