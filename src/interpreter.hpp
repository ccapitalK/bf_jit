#pragma once

#include <cstdint>
#include <vector>

#include "arguments.hpp"
#include "ir.hpp"

template <typename CellType>
void interpret(const std::vector<Instruction> &prog, std::vector<CellType> &bfMem, const Arguments &args);
