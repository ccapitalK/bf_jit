#pragma once

#include <string>
#include <vector>

enum class GetCharBehaviour { EOF_RETURNS_0, EOF_RETURNS_255, EOF_DOESNT_MODIFY };

struct Arguments {
    size_t bfMemLength;
    std::vector<std::string> fileNames;
    bool verbose{false};
    bool dumpCode{false};
    bool genSyms{false};
    bool useInterpreter{false};
    bool noFlush{false};
    GetCharBehaviour getCharBehaviour{};
    Arguments(int argc, char *argv[]);
};
