#pragma once

#include <libelf.h>
#include <string>

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
private:
    int fd_;
    Elf *elfHandle_;
    std::string fileName_;
};