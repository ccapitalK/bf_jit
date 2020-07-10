#pragma once
#include "error.hpp"
#include <cstring>
#include <iostream>
#include <string_view>
#include <sys/mman.h>

const int PAGE_SIZE = 4096;

class ASMBuf {
    size_t used;
    size_t buf_len;
    bool is_exec;
    unsigned char *data;

  public:
    ASMBuf(size_t pages) : used(0), buf_len(pages * PAGE_SIZE), is_exec(false) {
        data = static_cast<unsigned char *>(
            mmap(nullptr, buf_len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0));
        if (data == MAP_FAILED) {
            throw JITError("Failed to mmap ASMBuf: ", strerror(errno));
        }
    }
    ASMBuf(const ASMBuf &other) = delete;
    ASMBuf(ASMBuf &&other) : used(other.used), buf_len(other.buf_len), is_exec(other.is_exec), data(other.data) {
        other.data = nullptr;
    }
    ~ASMBuf() {
        if (data != nullptr) {
            munmap(data, buf_len);
            data = nullptr;
        }
    }
    size_t length() const noexcept { return used; }
    void grow() {
        // create new mapping
        auto old_data = data;
        data = static_cast<unsigned char *>(
            mmap(nullptr, 2 * buf_len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0));
        if (data == MAP_FAILED) {
            throw JITError("Failed to grow ASMBuf: ", strerror(errno));
        }
        // copy data
        memcpy(data, old_data, used);
        // remove old mapping
        munmap(old_data, buf_len);
        // synchronize attributes
        buf_len *= 2;
        set_executable(is_exec);
    }
    void *get_offset(size_t off) const { return static_cast<void *>(data + off); }
    void set_executable(bool executable) {
        if (data == nullptr)
            return;
        int prot;
        if (executable) {
            prot = PROT_READ | PROT_EXEC;
        } else {
            prot = PROT_READ | PROT_WRITE;
        }
        mprotect(data, buf_len, prot);
        is_exec = executable;
    }
    void write_bytes(std::initializer_list<unsigned char> bytes) {
        if (is_exec) {
            throw JITError("Tried to write byte to executable section");
        }
        for (auto byte : bytes) {
            if (used >= buf_len) {
                grow();
                // throw JITError("Wrote past the end of asm_buf");
            }
            data[used++] = byte;
        }
    }
    void write_str(const std::string_view s) {
        if (is_exec) {
            throw JITError("Tried to write byte to executable section");
        }
        while (used + s.length() > buf_len) {
            grow();
        }
        memcpy(data + used, s.data(), s.length());
        used += s.length();
    }
    template <typename T> void write_val(T val) {
        constexpr size_t len = sizeof(T);
        if (is_exec) {
            throw JITError("Tried to write byte to executable section");
        }
        while (used + len > buf_len) {
            grow();
        }
        for (size_t i = 0; i < len; ++i) {
            unsigned char byte = (val >> (8 * i)) & 0xff;
            data[used++] = byte;
        }
    }
    template <typename T> void patch_val(size_t pos, T val) {
        size_t old_used = used;
        used = pos;
        write_val(val);
        used = old_used;
    }
    void print_buf_instructions() const {
        for (size_t i = 0; i < used; ++i) {
            std::cout.fill('0');
            std::cout.width(2);
            std::cout << std::hex << static_cast<unsigned int>(data[i]);
        }
        std::cout << '\n';
    }
    size_t current_offset() const { return used; }
};

void enter_buf(void *addr);
