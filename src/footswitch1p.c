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
#include <inttypes.h>
#include <unistd.h>
#include <hidapi.h>
#include "common.h"
#include "debug.h"

hid_device *dev = NULL;

#define PEDAL_PIN_3_P15 0x03

enum pedal_report_ids {
    REPORT_DEVICE_ID   = 0x22,
    REPORT_SET_CODE    = 0x10,
};

typedef struct pedal_data {
    union {
        struct {
            unsigned char report_id;
            unsigned char pin;
            unsigned char command;
            unsigned char size;
            unsigned char data[8];
        };
        unsigned char buffer[64];
    };
} pedal_data_t;

pedal_data_t pd = { 0 };

void usage() {
    fprintf(stderr, "Usage: footswitch1p [-r] [-k <key>] [-m <modifier>] [-b <button>] [-xyw <XYW>]\n"
        "   -r          - read all pedals\n"
        "   -k key      - write the specified key\n"
        "   -m modifier - (l_,r_)ctrl|shift|alt|win\n"
        "   -b button   - mouse_left|mouse_middle|mouse_right\n"
        "   -x X        - move the mouse cursor horizontally by X pixels\n"
        "   -y Y        - move the mouse cursor vertically by Y pixels\n"
        "   -w W        - move the mouse wheel by W\n\n"
        "You cannot mix -km options with -bxyw options.\n");
    exit(1);
}

void init_pid(unsigned short vid, unsigned short pid) {
#ifdef OSX
    hid_init();
    dev = hid_open(vid, pid, NULL);
#else
    struct hid_device_info *info = NULL, *ptr = NULL;
    hid_init();
    info = hid_enumerate(vid, pid);
    ptr = info;
    while (ptr != NULL) {
        if (ptr->interface_number == 3) {
            dev = hid_open_path(ptr->path);
            break;
        }
        ptr = ptr->next;
    }
    hid_free_enumeration(info);
#endif
}

void init() {
    static unsigned short vid_pid[][2] = {
        {0x5131, 0x2019},
    };

    for (size_t i = 0 ; i < sizeof(vid_pid) / sizeof(vid_pid[0]) ; i++) {
        init_pid(vid_pid[i][0], vid_pid[i][1]);
        if (dev != NULL) {
            break;
        }
    }
    if (dev == NULL) {
        fatal("Cannot find footswitch with one of the supported VID:PID.\nCheck that the device is connected and that you have the correct permissions to access it.");
    }
}

void init_pedal() {
    pd.report_id = REPORT_SET_CODE;
    pd.pin = PEDAL_PIN_3_P15;
}

void deinit() {
    hid_close(dev);
    hid_exit();
}

void usb_write(pedal_data_t *pd) {
    int r = hid_write(dev, pd->buffer, sizeof(pd->buffer));
    if (r < 0) {
        fatal("error writing data (%ls)", hid_error(dev));
    }
    usleep(30 * 1000);
}

void read_pedals() {
    pedal_data_t response = { .buffer = { 0 } };

    pd.report_id = REPORT_DEVICE_ID,
    pd.pin = 0x00;
    pd.command = 0x00;
    pd.size = 0x22;

    usb_write(&pd);
    int r = hid_read(dev, response.buffer, sizeof(response.buffer));
    if (r < 0) {
        fatal("error reading data (%ls)", hid_error(dev));
    }

    if (response.report_id == REPORT_DEVICE_ID) {
        printf("Device ID: %" PRIu64 "\n" , *(uint64_t*)&response.data);
    } else {
        fprintf(stderr, "Unknown response:\n");
    }
}

void compile_key(const char *key) {
    unsigned char b = 0;

    if (!encode_key(key, &b)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }

    pd.command = 0x80;
    pd.size = 0x08;
    pd.data[2] = b;
}

void compile_modifier(const char *mod_str) {
    enum modifier mod;

    if (!parse_modifier(mod_str, &mod)) {
        fprintf(stderr, "Invalid modifier '%s'\n", mod_str);
        exit(1);
    }

    pd.command = 0x80;
    pd.size = 0x08;
    pd.data[0] |= mod;
}

void compile_mouse_button(const char *btn_str) {
    enum mouse_button btn;

    if (!parse_mouse_button(btn_str, &btn)) {
        fprintf(stderr, "Invalid mouse button '%s'\n", btn_str);
        exit(1);
    }

    pd.command = 0x02;
    pd.size = 0x04;
    pd.data[0] |= (btn | 0x8);
}

void compile_mouse_xyw(const char *mx, const char *my, const char *mw) {

    pd.command = 0x02;
    pd.size = 0x04;
    pd.data[0] |= 0x8;

    if (mx) {
        int x = atoi(mx);
        if (x < -128 || x > 127) {
            fprintf(stderr, "'x' must be in [-128, 127]\n");
            exit(1);
        }

        pd.data[1] = -x;
    }
    if (my) {
        int y = atoi(my);
        if (y < -128 || y > 127) {
            fprintf(stderr, "'y' must be in [-128, 127]\n");
            exit(1);
        }

        pd.data[2] = -y;
    }
    if (mw) {
        int w = atoi(mw);
        if (w < -128 || w > 127) {
            fprintf(stderr, "'w' must be in [-128, 127]\n");
            exit(1);
        }
        w = w < 0 ? 256 + w : w;
        pd.data[3] = w;
    }
}

void write_pedals() {
    /*
    printf("start: \n");
    printf("pedal header: \n");
    printf("report id: %02x\n", pd.report_id);
    printf("pin: %02x\n", pd.pin);
    printf("command: %02x\n", pd.command);
    printf("data size: %02x\n", pd.size);
    printf("pedal data: ");
    debug_arr(pd.data, pd.size);
    */

    usb_write(&pd);
}

int main(int argc, char *argv[]) {
    int opt;

    if (argc == 1) {
        usage();
    }

    if (argc == 2 && strcmp(argv[1], "-r") == 0) {
        init();
        read_pedals();
        deinit();
        return 0;
    }

    init_pedal();
    while ((opt = getopt(argc, argv, "rk:m:b:x:y:w:")) != -1) {
        switch (opt) {
            case 'r':
                fprintf(stderr, "Cannot use -r with other options\n");
                return 1;
            case 'k':
                compile_key(optarg);
                break;
            case 'm':
                compile_modifier(optarg);
                break;
            case 'b':
                compile_mouse_button(optarg);
                break;
            case 'x':
                compile_mouse_xyw(optarg, NULL, NULL);
                break;
            case 'y':
                compile_mouse_xyw(NULL, optarg, NULL);
                break;
            case 'w':
                compile_mouse_xyw(NULL, NULL, optarg);
                break;
            default:
                usage();
                break;
        }
    }

    init();
    write_pedals();
    deinit();

    return 0;
}

