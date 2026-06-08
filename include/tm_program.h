#pragma once

#include <string>
#include <vector>

#include "rule.h"

struct TMProgram {
    std::string startState;
    std::vector<Rule> rules;
};