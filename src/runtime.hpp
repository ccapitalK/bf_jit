#pragma once

#include "arguments.hpp"

using GetCharFunc = unsigned char (*)(int);
using PutCharFunc = int (*)(int);

extern "C" {
unsigned char mgetchar_0_on_eof(int);
unsigned char mgetchar_255_on_eof(int);
unsigned char mgetchar_nothing_on_eof(int current_cell);
int mputchar(int c);
int mputchar_noflush(int c);
}

GetCharFunc getCharFunc(const Arguments &args);
PutCharFunc putCharFunc(const Arguments &args);
