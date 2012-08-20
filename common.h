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


#define VID 0x0C45
#define PID 0x7403

enum action {
    NONE = -1,
    READ,
    STRING,
    RAW_STRING,
    COMBO,
    MOUSE,
};

enum modifier {
    INVALID = -1,
    CTRL = 1,
    SHIFT = 2,
    ALT = 4,
    WIN = 8,
    MOUSE_LEFT = 16,
    MOUSE_RIGHT = 32,
    MOUSE_MIDDLE = 64,
};

typedef unsigned char Bool;

enum modifier parse_modifier(const char *arg);

Bool encode_string(const char *str, unsigned char *arr);

Bool encode_key(const char *key, unsigned char *b);

Bool decode_byte(unsigned char b, char *str);

#endif
