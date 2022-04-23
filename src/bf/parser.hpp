#pragma once

#include <iostream>
#include <stack>
#include <vector>

#include "arguments.hpp"
#include "asmbuf.hpp"
#include "error.hpp"
#include "ir.hpp"

class Parser {
  public:
    Parser() = delete;
    explicit Parser(const Arguments &args);
    void checkNotFinished() const;
    void feed(std::istream &is);
    std::vector<Instruction> compile();

  private:
    std::vector<Instruction> outStream_;
    std::stack<int> loopStack_;
    size_t loopCount_{};
    bool compiled_{false};
    bool verbose_;
};
