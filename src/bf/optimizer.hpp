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
    bool multPass();
    bool optimizePass();

    bool verbose_;
    std::vector<Instruction> *progPtr_{nullptr};
};
