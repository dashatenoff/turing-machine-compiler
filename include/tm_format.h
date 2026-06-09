#pragma once

#include <string>

#include "tm_program.h"

class TMFormat {
public:
    static bool save(const std::string& filename, const TMProgram& program);
    static bool load(const std::string& filename, TMProgram& program);

}