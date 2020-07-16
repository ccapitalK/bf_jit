#pragma once

#include <vector>

#include "arguments.hpp"
#include "ir.hpp"

void interpret(const std::vector<Instruction> &prog, std::vector<char> &bfMem, Arguments& args);
