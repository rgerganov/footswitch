/*
Copyright (c) 2012 Radoslav Gerganov <rgerganov@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdbool.h>

enum modifier {
    CTRL = 1,
    SHIFT = 2,
    ALT = 4,
    WIN = 8,
    R_CTRL = 16,
    R_SHIFT = 32,
    R_ALT = 64,
    R_WIN = 128,
};

enum mouse_button {
    MOUSE_LEFT = 1,
    MOUSE_RIGHT = 2,
    MOUSE_MIDDLE = 4,
    MOUSE_DOUBLE = 8,
};

bool parse_modifier(const char *arg, enum modifier *mod);
bool parse_mouse_button(const char *arg, enum mouse_button *btn);

bool encode_string(const char *str, unsigned char *arr);
bool encode_char(const char ch, unsigned char *b);
bool encode_key(const char *key, unsigned char *b);

const char* decode_byte(unsigned char b);

#endif
