#include <cstdio>
#include <iostream>

#include "runtime.hpp"

unsigned char mgetc() {
    int c = getchar();
    if (c == EOF) {
        std::cerr << "\nError: Unexpected end of input" << std::endl;
        exit(1);
    }
    return c;
}

int mputchar(int c) {
    fflush(stdout);
    return putchar(c);
}
