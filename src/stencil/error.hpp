#pragma once
#include <exception>
#include <sstream>

// Main error type for stencil compilation errors
class StencilCompilerError : public std::runtime_error {
    std::stringstream ss;
    template <typename T, typename... R> void append_strs(const T &first, R... rest) {
        ss << first;
        append_strs(rest...);
    }
    void append_strs() {}

  public:
    template <typename... R> StencilCompilerError(R... il) : std::runtime_error("") {
        append_strs(il...);
        static_cast<std::runtime_error &>(*this) = std::runtime_error(ss.str());
    }
};
