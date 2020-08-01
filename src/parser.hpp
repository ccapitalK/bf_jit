#pragma once

#include <cassert>
#include <iostream>
#include <optional>
#include <stack>
#include <unordered_map>
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
    bool constPropagatePass();
    bool deadCodeEliminationPass();
    // Pre: the code at [start...end) contains a loop (including []), that can be converted into mults
    //      i.e. balanced count of < and >, single - or + at root, only invalid/add/adp instructions inside
    void rewriteMultLoop(size_t start, size_t end);
    bool makeMultPass();
    bool optimize();

    std::vector<Instruction> outStream_;
    std::stack<int> loopStack_;
    size_t loopCount_{};
    bool compiled_{false};
    bool verbose_;
};
