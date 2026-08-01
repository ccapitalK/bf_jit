#pragma once

#include "arguments.hpp"

using GetCharFunc = unsigned int (*)(int);
using PutCharFunc = int (*)(int);

extern "C" {
unsigned int mgetchar_0_on_eof(int);
unsigned int mgetchar_255_on_eof(int);
unsigned int mgetchar_nothing_on_eof(int current_cell);
int mputchar(int c);
int mputchar_noflush(int c);
}

GetCharFunc getCharFunc(const Arguments &args);
PutCharFunc putCharFunc(const Arguments &args);
