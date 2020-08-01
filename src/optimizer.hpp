#pragma once

#include <vector>

#include "arguments.hpp"
#include "error.hpp"
#include "ir.hpp"

class Optimizer {
  public:
    Optimizer() = delete;
    Optimizer(const Arguments &arguments);
    void optimize(std::vector<Instruction> &program);

  private:
    std::vector<Instruction> &prog();
    bool constPropagatePass();
    bool deadCodeEliminationPass();
    // Pre: the code at [start...end) contains a loop (including []), that can be converted into mults
    //      i.e. balanced count of < and >, single - or + at root, only invalid/add/adp instructions inside
    void rewriteMultLoop(size_t start, size_t end);
    bool makeMultPass();
    bool optimizePass();

    bool verbose_;
    std::vector<Instruction> *progPtr_{nullptr};
};
