#include <cstdio>
#include <iostream>

#include "runtime.hpp"

unsigned char mgetc() {
    int c = getchar();
    if (c == EOF) {
        c = 0;
    }
    return c;
}

int mputchar(int c) {
    fflush(stdout);
    return putchar(c);
}
