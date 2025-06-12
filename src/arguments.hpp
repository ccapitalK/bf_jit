#pragma once
#include <vector>
#include <string>

enum class GetCharBehaviour { EOF_RETURNS_0, EOF_RETURNS_255, EOF_DOESNT_MODIFY };

struct Arguments {
    size_t bfMemLength;
    size_t cellBitWidth;
    std::vector<std::string> fileNames;
    bool verbose{false};
    bool dryRun{false};
    bool dumpCode{false};
    bool dumpMem{false};
    bool genSyms{false};
    bool useInterpreter{false};
    bool noFlush{false};
    GetCharBehaviour getCharBehaviour{GetCharBehaviour::EOF_RETURNS_0};

    Arguments(int argc, char *argv[]);

private:
    void printUsage(const char* progName);
};
