#pragma once

#include <libelf.h>
#include <string>
#include <vector>

struct RuntimePatch {
    std::string symbolName;
    size_t offset;
};

struct CodeBlob {
    std::vector<std::byte> data;
    std::vector<RuntimePatch> patches;
};

class ElfParser {
public:
    ElfParser() = delete;
    ElfParser(const ElfParser&) = delete;
    ElfParser(ElfParser&&) = delete;
    ElfParser& operator=(const ElfParser&) = delete;
    ElfParser& operator=(ElfParser&&) = delete;

    ElfParser(const char *filename);
    ~ElfParser() noexcept;
    const char* getFilename();
    const char* getElfKind();
    size_t countFunctions();
private:
    int fd_;
    Elf *elfHandle_;
    std::string fileName_;
};