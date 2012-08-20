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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "common.h"

#define KEYS_SIZE 31
#define CHARS_SIZE 34

static const char *keys[KEYS_SIZE] = {
    "enter",
    "esc",
    "backspace",
    "tab",
    "space",
    "capslock",
    "f1",
    "f2",
    "f3",
    "f4",
    "f5",
    "f6",
    "f7",
    "f8",
    "f9",
    "f10",
    "f11",
    "f12",
    "printscreen",
    "scrollock",
    "pause",
    "insert",
    "home",
    "pageup",
    "delete",
    "pagedown",
    "right",
    "down",
    "left",
    "up",
    "numlock",
};

static const unsigned char key_codes[KEYS_SIZE] = {
    0x28, //"enter",
    0x29, //"esc",
    0x2a, //"backspace",
    0x2b, //"tab",
    0x2c, //"space",
    0x39, //"capslock",
    0x3a, //"f1",
    0x3b, //"f2",
    0x3c, //"f3",
    0x3d, //"f4",
    0x3e, //"f5",
    0x3f, //"f6",
    0x40, //"f7",
    0x41, //"f8",
    0x42, //"f9",
    0x43, //"f10",
    0x44, //"f11",
    0x45, //"f12",
    0x46, //"printscreen",
    0x47, //"scrollock",
    0x48, //"pause",
    0x49, //"insert",
    0x4a, //"home",
    0x4b, //"pageup",
    0x4c, //"delete",
    0x4e, //"pagedown",
    0x4f, //"right",
    0x50, //"down",
    0x51, //"left",
    0x52, //"up",
    0x53, //"numlock",
};

static const unsigned char chars[CHARS_SIZE][2] = {
    {' ', 0x2c},
    {'0', 0x27},
    {'!', 0x9e},
    {'@', 0x9f},
    {'#', 0xa0},
    {'$', 0xa1},
    {'%', 0xa2},
    {'^', 0xa3},
    {'&', 0xa4},
    {'*', 0xa5},
    {'(', 0xa6},
    {')', 0xa7},
    {'-', 0x2d},
    {'_', 0xad},
    {'=', 0x2e},
    {'+', 0xae},
    {'[', 0x2f},
    {'{', 0xaf},
    {']', 0x30},
    {'}', 0xb0},
    {'\\',0x31},
    {'|', 0xb1},
    {';', 0x33},
    {':', 0xb3},
    {'\'',0x34},
    {'"', 0xb4},
    {'`', 0x35},
    {'~', 0xb5},
    {',', 0x36},
    {'<', 0xb6},
    {'.', 0x37},
    {'>', 0xb7},
    {'/', 0x38},
    {'?', 0xb8},
};

enum modifier parse_modifier(const char *arg) {
    if (strcasecmp("ctrl", arg) == 0) {
        return CTRL;
    } else if (strcasecmp("alt", arg) == 0) {
        return ALT;
    } else if (strcasecmp("win", arg) == 0) {
        return WIN;
    } else if (strcasecmp("shift", arg) == 0) {
        return SHIFT;
    } else if (strcasecmp("mouse_left", arg) == 0) {
        return MOUSE_LEFT;
    } else if (strcasecmp("mouse_middle", arg) == 0) {
        return MOUSE_MIDDLE;
    } else if (strcasecmp("mouse_right", arg) == 0) {
        return MOUSE_RIGHT;
    } else {
        return INVALID;
    }
}

static Bool encode_char(const char ch, unsigned char *b) {
    int i;
    if (ch >= 'a' && ch <= 'z') {
        *b = ch - 0x5d;
        return 1;
    } else if (ch >= 'A' && ch <= 'Z') {
        *b = ch + 0x43;
        return 1;
    } else if (ch >= '1' && ch <= '9') {
        *b = ch - 0x13;
        return 1;
    }
    for (i = 0 ; i < CHARS_SIZE ; i++) {
        if (ch == chars[i][0]) {
            *b = chars[i][1];
            return 1;
        }
    }
    return 0;
}

Bool encode_string(const char *str, unsigned char *arr) {
    int i;
    for (i = 0 ; i < strlen(str) ; i++) {
        if (!encode_char(str[i], &arr[i])) {
            return 0;
        }
    }
    return 1;
}

Bool encode_key(const char *key, unsigned char *b) {
    int i;
    if (strlen(key) == 1) {
        return encode_char(tolower(key[0]), b);
    }
    for (i = 0 ; i < KEYS_SIZE ; i++) {
        if (strcasecmp(keys[i], key) == 0) {
            *b = key_codes[i];
            return 1;
        }
    }
    return 0;
}

Bool decode_byte(unsigned char b, char *str) {
    int i;
    if (b >= 4 && b <= 0x1d) {
        str[0] = 'a' + (b-4);
        str[1] = 0;
        return 1;
    }
    if (b >= 0x84 && b <= 0x9d) {
        str[0] = 'A' + (b-0x84);
        str[1] = 0;
        return 1;
    }
    if (b >= 0x1e && b <= 0x26) {
        str[0] = '1' + (b-0x1e);
        str[1] = 0;
        return 1;
    }
    for (i = 0 ; i < KEYS_SIZE ; i++) {
        if (key_codes[i] == b) {
            sprintf(str, "<%s>", keys[i]);
            return 1;
        }
    }
    for (i = 0 ; i < CHARS_SIZE ; i++) {
        if (chars[i][1] == b) {
            str[0] = chars[i][0];
            str[1] = 0;
            return 1;
        }
    }
    sprintf(str, "<0x%X>", b);
    return 0;
}

