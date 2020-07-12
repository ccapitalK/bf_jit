#include "asmbuf.hpp"

void enter_buf(const void *addr) { asm("call *%0" : : "r"(addr)); }
