#include <cstdio>
#include <iostream>

#include "runtime.hpp"

unsigned char mgetchar_0_on_eof() {
    int c = getchar();
    if (c == EOF) {
        c = 0;
    }
    return c;
}

unsigned char mgetchar_255_on_eof() {
    int c = getchar();
    if (c == EOF) {
        c = 0;
    }
    return c;
}

unsigned char mgetchar_nothing_on_eof() {
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

int mputchar_noflush(int c) {
    return putchar(c);
}
