#include <cstdio>

#include "error.hpp"
#include "runtime.hpp"

unsigned char mgetchar_0_on_eof(int) {
    int c = getchar();
    return c == EOF ? 0 : c;
}

unsigned char mgetchar_255_on_eof(int) {
    int c = getchar();
    return c == EOF ? 255 : c;
}

unsigned char mgetchar_nothing_on_eof(int current_cell) {
    int c = getchar();
    return c == EOF ? current_cell : c;
}

int mputchar(int c) {
    fflush(stdout);
    return putchar(c);
}

int mputchar_noflush(int c) { return putchar(c); }

GetCharFunc getCharFunc(const Arguments &args) {
    switch (args.getCharBehaviour) {
    case GetCharBehaviour::EOF_RETURNS_0:
        return mgetchar_0_on_eof;
    case GetCharBehaviour::EOF_RETURNS_255:
        return mgetchar_255_on_eof;
    case GetCharBehaviour::EOF_DOESNT_MODIFY:
        return mgetchar_nothing_on_eof;
    default:
        throw JITError("Unknown GetCharBehaviour variant: ", (int)args.getCharBehaviour);
    }
}

PutCharFunc putCharFunc(const Arguments &args) {
    if (args.noFlush) {
        return mputchar_noflush;
    } else {
        return mputchar;
    }
}
