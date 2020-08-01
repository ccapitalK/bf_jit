#pragma once

#include <vector>

#include "arguments.hpp"
#include "error.hpp"
#include "optimizer.hpp"
#include "parser.hpp"

template <typename CellType> class Engine {
  public:
    Engine() = delete;
    explicit Engine(const Arguments &args);
    void run();

  private:
    static constexpr size_t RDBUF_SIZE = 256 * 1024;
    std::vector<char> rdbuf_;
    const Arguments &arguments_;
    std::vector<CellType> bfMem_;
    Optimizer optimizer_;
    Parser parser_{arguments_};
};
