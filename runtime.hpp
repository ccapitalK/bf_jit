#pragma once

extern "C" {
unsigned char mgetchar_0_on_eof();
unsigned char mgetchar_255_on_eof();
unsigned char mgetchar_nothing_on_eof();
int mputchar(int c);
int mputchar_noflush(int c);
}
