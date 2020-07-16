#pragma once

#include <string>
#include <vector>

struct Arguments {
    size_t bfMemLength;
    std::vector<std::string> fileNames;
    bool verbose{false};
    bool dumpCode{false};
    bool genSyms{false};
    bool useInterpreter{false};
    Arguments(int argc, char *argv[]);
};
