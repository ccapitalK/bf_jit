#include "asmbuf.hpp"

void enter_buf(void *addr) {
    asm("call *%0" : : "r" (addr));
}
