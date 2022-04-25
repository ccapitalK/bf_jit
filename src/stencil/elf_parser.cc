#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libelf.h>
#include <gelf.h>
#include <unistd.h>

#include "elf_parser.hpp"
#include "error.hpp"

static void ensure_elf_initialized() {
    static bool initialized = false;
    if (!initialized) {
        elf_version(EV_CURRENT);
        initialized = true;
    }
}

ElfParser::ElfParser(const char *filename)
  : fd_(-1)
  , fileName_(filename)
  , elfHandle_(nullptr)
{
    ensure_elf_initialized();
    fd_ = open(filename, O_RDONLY, 0);
    if (fd_ < 0) {
        int error = errno;
        throw StencilCompilerError("Failed to open \"", filename, "\": ", strerror(error));
    }
    elfHandle_ = elf_begin(fd_, ELF_C_READ, nullptr);
    if (elfHandle_ == nullptr) {
        throw StencilCompilerError("elf_begin(\"", filename, "\") failed: ", elf_errmsg(elf_errno()));
    }
}

ElfParser::~ElfParser() noexcept {
    if (elfHandle_) {
        elf_end(elfHandle_);
    }
    if (fd_ >= 0) {
        close(fd_);
    }
}

const char* ElfParser::getFilename() {
    return fileName_.c_str();
}

const char* ElfParser::getElfKind() {
    Elf_Kind kind = elf_kind(elfHandle_);
    switch(kind) {
    case ELF_K_AR:
        return "ar(1) archive";
    case ELF_K_ELF:
        return "elf object";
    case ELF_K_NONE:
        return "data";
    default:
        return "unknown";
    }
}

size_t ElfParser::countFunctions() {
    GElf_Ehdr ehdr;
    if (!gelf_getehdr(elfHandle_, &ehdr)) {
        throw StencilCompilerError("Failed to read EHdr: ", elf_errmsg(elf_errno()));
    }
    int elfClass = gelf_getclass(elfHandle_);
    if (elfClass == ELFCLASSNONE) {
        throw StencilCompilerError("Invalid elfclass ELFCLASSNONE");
    }
    bool is64Bit = elfClass == ELFCLASS64;
    char *id = elf_getident(elfHandle_, nullptr);
    if (!id) {
        throw StencilCompilerError("elf_getident() failed: ", elf_errmsg(elf_errno()));
    }
    std::cout << id << "\n";
    return 0;
}