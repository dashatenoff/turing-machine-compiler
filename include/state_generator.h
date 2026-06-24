#pragma once

#include <string>
#include <unordered_set>
#include <stdexcept>

//stateGenerator выдаёт уникальные имена состояний машины тьюринга
class StateGenerator {
public:
    //HALT зарезервирован как конечное состояние
    static constexpr const char* HALT  = "HALT";
    static constexpr const char* START = "start";

    std::string next(const std::string& prefix) {
        if (prefix == HALT || prefix == START) {
            throw std::runtime_error(
                    "StateGenerator: имя '" + prefix + "' зарезервировано"
            );
        }
        std::string name = prefix + "_" + std::to_string(counter_++);
        used_.insert(name);
        return name;
    }

    //зарезервированные имена выдаются напрямую без счётчика они всегда одни и те же
    static std::string halt()  { return HALT;  }
    static std::string start() { return START; }

private:
    int                          counter_ = 0;
    std::unordered_set<std::string> used_;//хэш множетсво уник сост
};