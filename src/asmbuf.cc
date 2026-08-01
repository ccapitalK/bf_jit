#include "asmbuf.hpp"

void enter_buf(const void *addr) {
    // Call into addr, which is a function pointer
    ((void(*)(void))addr)();
}
