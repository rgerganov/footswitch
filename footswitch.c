/*
Copyright (c) 2012 Radoslav Gerganov <rgerganov@gmail.com>
Copyright (c) 2012 Daniel Manjarres <danmanj@gmail.com>

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "common.h"
#include "debug.h"

libusb_context *ctx = NULL;
libusb_device_handle *dev = NULL;
unsigned char driver_detached = 0;

const int KEYBOARD_MODS = CTRL | SHIFT | ALT | WIN;
const int MOUSE_MODS = MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT;

void usage(char *argv0) {
    fprintf(stderr, "Usage: %s [-r] [-s <string>] [-S <raw_string>] [-k <key>] [-m <modifier>] [-x <X>] [-y <Y>] [-w <W>]\n"
        "   -r          - read the persisted function\n"
        "   -s string   - write the specified string\n"
        "   -S rstring  - write the specified raw string (hex numbers delimited with spaces)\n"
        "   -k key      - write the specified key; can be used with one or several -m\n"
        "   -m modifier - ctrl|shift|alt|win|mouse_left|mouse_middle|mouse_right \n"
        "   -x X        - move the mouse cursor horizontally by X pixels\n"
        "   -y Y        - move the mouse cursor vertically by Y pixels\n"
        "   -w W        - move the mouse wheel by W\n", argv0);
    exit(1);
}

void init() {
    int r = libusb_init(&ctx);
    if (r < 0) {
        fatal("cannot initialize libusb");
    }
    libusb_set_debug(ctx, 3);
    dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (dev == NULL) {
        fatal("cannot find footswitch with VID:PID=%x:%x", VID, PID);
    }
    r = libusb_kernel_driver_active(dev, 1);
    if (r < 0) {
        fatal("check if driver is active, error: %s", libusb_err(r));
    }
    if (r == 1) {
        r = libusb_detach_kernel_driver(dev, 1);
        if (r < 0) {
            fatal("cannot detach kernel driver, error: %s", libusb_err(r));
        }
        driver_detached = 1;
    }
    r = libusb_claim_interface(dev, 1);
    if (r < 0) {
        fatal("cannot claim interface, error: %s", libusb_err(r));
    }
}

void deinit() {
    int r = libusb_release_interface(dev, 1);
    if (r < 0) {
        fatal("cannot release interface, error: %s", libusb_err(r));
    }
    if (driver_detached) {
        libusb_attach_kernel_driver(dev, 1);
    }
    libusb_close(dev);
    libusb_exit(ctx);
}

void usb_write(unsigned char data[8]) {
    int r = libusb_control_transfer(dev, 0x21, 0x09, 0x200, 0x1, data, 8, 500);
    if (r < 0) {
        fatal("error writing data (%s)", libusb_err(r));
    }
}

void write_magic_begin() {
    unsigned char wrt1[8] = {0x01, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char wrt2[8] = {0x01, 0x81, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00};
    unsigned char wrt3[8] = {0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    usb_write(wrt1);
    usleep(50 * 1000);
    usb_write(wrt2);
    usleep(50 * 1000);
    usb_write(wrt3);
    usleep(50 * 1000);
}

void write_magic_end() {
    unsigned char wrt4[8] = {0x01, 0x81, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00};
    unsigned char wrt5[8] = {0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    usb_write(wrt4);
    usleep(50 * 1000);
    usb_write(wrt5);
    usleep(50 * 1000);
}

void read_mouse(unsigned char data[]) {
    int x = data[5], y = data[6], w = data[7];
    switch (data[4]) {
        case 1:
            printf("mouse_left ");
            break;
        case 2:
            printf("mouse_right ");
            break;
        case 4:
            printf("mouse_middle ");
            break;
    }
    x = x > 127 ? x - 256 : x;
    y = y > 127 ? y - 256 : y;
    w = w > 127 ? w - 256 : w;
    printf("X=%d Y=%d W=%d\n", x, y, w);
}

void read_combo(unsigned char data[]) {
    char combo[128] = {0};
    if ((data[2] & 1) != 0) {
        strcat(combo, "ctrl+");
    }
    if ((data[2] & 2) != 0) {
        strcat(combo, "shift+");
    }
    if ((data[2] & 4) != 0) {
        strcat(combo, "alt+");
    }
    if ((data[2] & 8) != 0) {
        strcat(combo, "win+");
    }
    switch (data[4]) {
        case 1:
            strcat(combo, "mouse_left+");
            break;
        case 2:
            strcat(combo, "mouse_right+");
            break;
        case 4:
            strcat(combo, "mouse_middle+");
            break;
    }
    if (data[3] != 0) {
        const char *key = decode_byte(data[3]);
        strcat(combo, key);
    } else {
        size_t len = strlen(combo);
        if (len > 0) {
            combo[len - 1] = 0; // remove the last +
        }
    }
    printf("%s\n", combo);
}

void read_string(unsigned char data[]) {
    int r = 0, tr = 0, ind = 2;
    int len = data[0] - 2;
    const char *str = NULL;

    while (len > 0) {
        if (ind == 8) {
            r = libusb_interrupt_transfer(dev, 0x82, data, 8, &tr, 500);
            if (r < 0) {
                fatal("error reading data (%s)", libusb_err(r));
            }
            if (tr != 8) {
                fatal("expected 8 bytes, received: %d", tr);
            }
            ind = 0;
        }
        str = decode_byte(data[ind]);
        printf("%s", str);
        len--;
        ind++;
    }
    printf("\n");
}

void read_usb() {
    int r = 0, tr = 0;
    unsigned char data[8] = {0x01, 0x82, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00};

    usb_write(data);
    r = libusb_interrupt_transfer(dev, 0x82, data, 8, &tr, 500);
    if (r < 0) {
        fatal("error reading data (%s)", libusb_err(r));
    }

    switch (data[1]) {
        case 1:
        case 0x81:
            read_combo(data);
            break;
        case 2:
            read_mouse(data);
            break;
        case 3:
            read_combo(data);
            break;
        case 4:
            read_string(data);
            break;
        default:
            fprintf(stderr, "Unknown data:\n");
            debug_arr(data, 8);
            return;
    }
}

void write_raw_data(unsigned char *arr, int len) {
    unsigned char data[8];
    int arr_ind = 0, data_ind = 0;

    write_magic_begin();
    memset(data, 0, 8);
    data[0] = 1;
    data[1] = 0x81;
    data[2] = len + 2;
    data[3] = 2;
    usb_write(data);
    memset(data, 0, 8);
    data[data_ind++] = len + 2;
    data[data_ind++] = 4;
    while (arr_ind < len) {
        if (data_ind == 8) {
            usb_write(data);
            memset(data, 0, 8);
            data_ind = 0;
        }
        data[data_ind++] = arr[arr_ind++];
    }
    usb_write(data);
    usleep(50 * 1000);
    write_magic_end();
}

void write_raw_string(const char *str) {
    unsigned char arr[40];
    int ind = 0;
    char *tok = strtok((char *)str, " ,");
    while (tok != NULL && ind < 38) {
        int val;
        if (sscanf(tok, "%x", &val) == 1) {
            arr[ind++] = val;
        } else {
            fprintf(stderr, "'%s' is invalid hex number\n", tok);
            return;
        }
        tok = strtok(NULL, " ,");
    }
    if (tok != NULL) {
        fprintf(stderr, "WARN: input string is truncated because it's too long\n");
    }
    write_raw_data(arr, ind);
    printf("success\n");
}

void write_string(const char *str) {
    unsigned char arr[40];
    size_t len = strlen(str);

    if (len > 38) {
        fprintf(stderr, "The size of the string must be <= 38\n");
        return;
    }
    if (!encode_string(str, arr)) {
        fprintf(stderr, "Cannot encode string: '%s'\n", str);
        return;
    }
    write_raw_data(arr, len);
    printf("success\n");
}

void write_combo(const char *key, int modifiers) {
    unsigned char b = 0;
    unsigned char data[8] = {0x01, 0x81, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00};
    //printf("write combo '%s', mods: %d\n", key, modifiers);
    if (key != NULL) {
        if (!encode_key(key, &b)) {
            fprintf(stderr, "Cannot encode key '%s'\n", key);
            return;
        }
    }
    write_magic_begin();
    usb_write(data);
    memset(data, 0, 8);
    data[0] = 8;
    data[1] = 1;
    data[3] = b;
    if (modifiers & KEYBOARD_MODS) {
        data[1] = 3;
        data[2] = modifiers & KEYBOARD_MODS;
        if (modifiers & MOUSE_LEFT) {
            data[4] = 1;
        } else if (modifiers & MOUSE_RIGHT) {
            data[4] = 2;
        } else if (modifiers & MOUSE_MIDDLE) {
            data[4] = 4;
        }
    }
    //debug_arr(data, 8);
    usb_write(data);
    write_magic_end();
    printf("success\n");
}

void write_mouse(int modifiers, const char *m_x, const char *m_y, const char *m_w) {
    unsigned char data[8] = {0x01, 0x81, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00};
    int x, y, w;

    x = m_x ? atoi(m_x) : 0;
    y = m_y ? atoi(m_y) : 0;
    w = m_w ? atoi(m_w) : 0;

    if (x < -128 || x > 127) {
        fprintf(stderr, "'x' must be in [-128, 127]\n");
        return;
    }
    if (y < -128 || y > 127) {
        fprintf(stderr, "'y' must be in [-128, 127]\n");
        return;
    }
    if (w < -128 || w > 127) {
        fprintf(stderr, "'w' must be in [-128, 127]\n");
        return;
    }
    x = x < 0 ? 256 + x : x;
    y = y < 0 ? 256 + y : y;
    w = w < 0 ? 256 + w : w;

    write_magic_begin();
    usb_write(data);
    memset(data, 0, 8);
    data[0] = 8;
    data[1] = 2;
    if (modifiers & MOUSE_LEFT) {
        data[4] = 1;
    } else if (modifiers & MOUSE_RIGHT) {
        data[4] = 2;
    } else if (modifiers & MOUSE_MIDDLE) {
        data[4] = 4;
    }
    data[5] = x;
    data[6] = y;
    data[7] = w;
    usb_write(data);
    write_magic_end();
    printf("success\n");
}


int main(int argc, char *argv[]) {
    int opt;
    enum action selected = NONE;
    enum modifier mod = INVALID;
    const char *string = NULL;
    const char *raw_string = NULL;
    const char *key = NULL;
    const char *m_x = NULL, *m_y = NULL, *m_w = NULL;
    int modifiers = 0;
    if (argc == 1) {
        usage(argv[0]);
    }
    while ((opt = getopt(argc, argv, "rs:S:k:m:x:y:w:")) != -1) {
        switch (opt) {
            case 'r':
                if (selected != NONE) {
                    usage(argv[0]);
                }
                selected = READ;
                break;
            case 's':
                if (selected != NONE) {
                    usage(argv[0]);
                }
                selected = STRING;
                string = optarg;
                break;
            case 'S':
                if (selected != NONE) {
                    usage(argv[0]);
                }
                selected = RAW_STRING;
                raw_string = optarg;
                break;
            case 'k':
                if (selected != NONE && selected != COMBO) {
                    usage(argv[0]);
                }
                if (key != NULL) {
                    usage(argv[0]);
                }
                selected = COMBO;
                key = optarg;
                break;
            case 'm':
                if (selected != NONE && selected != COMBO && selected != MOUSE) {
                    usage(argv[0]);
                }
                mod = parse_modifier(optarg);
                if (mod == INVALID) {
                    fprintf(stderr, "Invlalid modifier '%s'\n", optarg);
                    return 1;
                }
                if (KEYBOARD_MODS & mod) {
                    selected = COMBO;
                }
                modifiers |= mod;
                break;
            case 'x':
                if (selected != NONE && selected != MOUSE) {
                    usage(argv[0]);
                }
                if (m_x != NULL || modifiers & KEYBOARD_MODS) {
                    usage(argv[0]);
                }
                selected = MOUSE;
                m_x = optarg;
                break;
            case 'y':
                if (selected != NONE && selected != MOUSE) {
                    usage(argv[0]);
                }
                if (m_y != NULL || modifiers & KEYBOARD_MODS) {
                    usage(argv[0]);
                }
                selected = MOUSE;
                m_y = optarg;
                break;
            case 'w':
                if (selected != NONE && selected != MOUSE) {
                    usage(argv[0]);
                }
                if (m_w != NULL || modifiers & KEYBOARD_MODS) {
                    usage(argv[0]);
                }
                selected = MOUSE;
                m_w = optarg;
                break;
            default:
                usage(argv[0]);
                break;
        }
    }
    init();
    if (selected == NONE && (modifiers & MOUSE_MODS) != 0) {
        selected = MOUSE;
    }
    switch (selected) {
        case NONE:
            usage(argv[0]);
            break;
        case READ:
            read_usb();
            break;
        case STRING:
            write_string(string);
            break;
        case RAW_STRING:
            write_raw_string(raw_string);
            break;
        case COMBO:
            write_combo(key, modifiers);
            break;
        case MOUSE:
            write_mouse(modifiers, m_x, m_y, m_w);
            break;
    }
    deinit();
    return 0;
}
