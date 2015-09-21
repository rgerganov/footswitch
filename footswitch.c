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

typedef struct pedal_data
{
    unsigned char header[8];
    unsigned char data[48];
    int data_len;
} pedal_data;

typedef struct pedal_protocol
{
    unsigned char start[8];
    pedal_data pedals[3];
} pedal_protocol;

pedal_protocol pd = {{0}};
pedal_data *curr_pedal = &pd.pedals[1]; // start at the second pedal

// KEY and MOUSE types can be combined
#define KEY_TYPE    1
#define MOUSE_TYPE    2
// STRING must be by itself
#define STRING_TYPE    4

void usage() {
    fprintf(stderr, "Usage: footswitch [-123] [-r] [-s <string>] [-S <raw_string>] [-ak <key>] [-m <modifier>] [-b <button>] [-xyw <XYW>]\n"
        "   -r          - read all pedals\n"
        "   -1          - program the first pedal\n"
        "   -2          - program the second pedal (default)\n"
        "   -3          - program the third pedal\n"
        "   -s string   - append the specified string\n"
        "   -S rstring  - append the specified raw string (hex numbers delimited with spaces)\n"
        "   -a key      - append the specified key\n"
        "   -k key      - write the specified key\n"
        "   -m modifier - ctrl|shift|alt|win\n"
        "   -b button   - mouse_left|mouse_middle|mouse_right\n"
        "   -x X        - move the mouse cursor horizontally by X pixels\n"
        "   -y Y        - move the mouse cursor vertically by Y pixels\n"
        "   -w W        - move the mouse wheel by W\n\n"
        "You cannot mix -sSa options with -kmbxyw options for one and the same pedal\n");
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
    #ifndef darwin
        r = libusb_claim_interface(dev, 1);
        if (r < 0) {
            fatal("cannot claim interface, error: %s", libusb_err(r));
        }
    #endif
}

void init_pedal(pedal_data *p, int num) {
    unsigned char default_header[8] = {0x01, 0x81, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char default_data[8] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    memcpy(p->header, default_header, 8);
    p->header[3] = num + 1;

    memset(p->data, 0, sizeof(p->data));
    memcpy(p->data, default_data, 8);

    p->data_len = 8;
}

void init_pedals() {
    unsigned char start[8] = {0x01, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};

    memcpy(pd.start, start, 8);
    init_pedal(&pd.pedals[0], 0);
    init_pedal(&pd.pedals[1], 1);
    init_pedal(&pd.pedals[2], 2);
}

void deinit() {
    #ifndef darwin
        int r = libusb_release_interface(dev, 1);
        if (r < 0) {
            fatal("cannot release interface, error: %s", libusb_err(r));
        }
    #endif
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
    usleep(30 * 1000);
}

void print_mouse(unsigned char data[]) {
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
    printf("X=%d Y=%d W=%d", x, y, w);
}

void print_key(unsigned char data[]) {
    char combo[128] = {0};
    if ((data[2] & CTRL) != 0) {
        strcat(combo, "ctrl+");
    }
    if ((data[2] & SHIFT) != 0) {
        strcat(combo, "shift+");
    }
    if ((data[2] & ALT) != 0) {
        strcat(combo, "alt+");
    }
    if ((data[2] & WIN) != 0) {
        strcat(combo, "win+");
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
    printf("%s", combo);
}

void print_string(unsigned char data[]) {
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
        if (strlen(str) > 1) {
            printf("<%s>", str);
        } else {
            printf("%s", str);
        }
        len--;
        ind++;
    }
}

void read_pedals() {
    int r = 0, tr = 0, i = 0;
    unsigned char query[8] = {0x01, 0x82, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char response[8];

    for (i = 0 ; i < 3 ; i++) {
        query[3] = i + 1;
        usb_write(query);
        r = libusb_interrupt_transfer(dev, 0x82, response, 8, &tr, 500);
        if (r < 0) {
            fatal("error reading data (%s)", libusb_err(r));
        }
        printf("[switch %d]: ", i + 1);
        switch (response[1]) {
            case 0:
                printf("unconfigured");
                break;
            case 1:
            case 0x81:
                print_key(response);
                break;
            case 2:
                print_mouse(response);
                break;
            case 3:
                print_key(response);
                printf(" ");
                print_mouse(response);
                break;
            case 4:
                print_string(response);
                break;
            default:
                fprintf(stderr, "Unknown response:\n");
                debug_arr(response, 8);
                return;
        }
        printf("\n");
    }
}

/**
 * The following types are valid:
 *   KEY_TYPE,
 *   MOUSE_TYPE,
 *   KEY_TYPE | MOUSE_TYPE,
 *   STRING_TYPE
 */
Bool set_pedal_type(unsigned char new_type) {
    unsigned char *curr_type = &curr_pedal->data[1];
    // check if there is no type set (default)
    if (*curr_type == 0) {
        // set type and data_len
        *curr_type = new_type;
        if (new_type == STRING_TYPE) {
            curr_pedal->data_len = 2;
        }
        return 1;
    }
    // type is already set, check if we can add the new type
    switch (new_type) {
        case STRING_TYPE:
            return *curr_type == STRING_TYPE;
        case KEY_TYPE:
        case MOUSE_TYPE:
            if (*curr_type == STRING_TYPE) {
                return 0;
            }
            *curr_type |= new_type;
            return 1;
    }
    return 0;
}

void compile_string_data(unsigned char *data, size_t len) {
    if (curr_pedal->data_len + len > 40) {
        fprintf(stderr, "The size of the accumulated string must be <= 38\n");
        exit(1);
    }
    memcpy(&curr_pedal->data[curr_pedal->data_len], data, len);
    curr_pedal->data_len += len;
    curr_pedal->header[2] = curr_pedal->data_len;
    curr_pedal->data[0] = curr_pedal->data_len;
}

void compile_string(const char *str) {
    size_t len = strlen(str);
    unsigned char arr[40] = {0};

    if (!set_pedal_type(STRING_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    if (len > 38) {
        fprintf(stderr, "The size of each string must be <= 38\n");
        exit(1);
    }
    if (!encode_string(str, arr)) {
        fprintf(stderr, "Cannot encode string: '%s'\n", str);
        exit(1);
    }
    compile_string_data(arr, len);
}

void compile_string_key(const char *key) {
    unsigned char b;

    if (!set_pedal_type(STRING_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    if (!encode_key(key, &b)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }
    compile_string_data(&b, 1);
}

void compile_raw_string(const char *str) {
    unsigned char arr[40];
    int ind = 0;
    char *tok = NULL;

    if (!set_pedal_type(STRING_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    tok = strtok((char *)str, " ,");
    while (tok != NULL && ind < 38) {
        int val;
        if (sscanf(tok, "%x", &val) == 1) {
            arr[ind++] = val;
        } else {
            fprintf(stderr, "'%s' is invalid hex number\n", tok);
            exit(1);
        }
        tok = strtok(NULL, " ,");
    }
    if (tok != NULL) {
        fprintf(stderr, "The size of each string must be <= 38\n");
        exit(1);
    }
    compile_string_data(arr, ind);
}

void compile_key(const char *key) {
    unsigned char b = 0;

    if (!set_pedal_type(KEY_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    if (!encode_key(key, &b)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }
    curr_pedal->data[3] = b;
}

void compile_modifier(const char *mod_str) {
    enum modifier mod;

    if (!parse_modifier(mod_str, &mod)) {
        fprintf(stderr, "Invlalid modifier '%s'\n", mod_str);
        exit(1);
    }
    if (!set_pedal_type(KEY_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    curr_pedal->data[2] |= mod;
}

void compile_mouse_button(const char *btn_str) {
    enum mouse_button btn;

    if (!parse_mouse_button(btn_str, &btn)) {
        fprintf(stderr, "Invalid mouse button '%s'\n", btn_str);
        exit(1);
    }
    if (!set_pedal_type(MOUSE_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    curr_pedal->data[4] = btn;
}

void compile_mouse_xyw(const char *mx, const char *my, const char *mw) {
    if (!set_pedal_type(MOUSE_TYPE)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    if (mx) {
        int x = atoi(mx);
        if (x < -128 || x > 127) {
            fprintf(stderr, "'x' must be in [-128, 127]\n");
            exit(1);
        }
        x = x < 0 ? 256 + x : x;
        curr_pedal->data[5] = x;
    }
    if (my) {
        int y = atoi(my);
        if (y < -128 || y > 127) {
            fprintf(stderr, "'y' must be in [-128, 127]\n");
            exit(1);
        }
        y = y < 0 ? 256 + y : y;
        curr_pedal->data[6] = y;
    }
    if (mw) {
        int w = atoi(mw);
        if (w < -128 || w > 127) {
            fprintf(stderr, "'w' must be in [-128, 127]\n");
            exit(1);
        }
        w = w < 0 ? 256 + w : w;
        curr_pedal->data[7] = w;
    }
}

void write_pedal(pedal_data *pedal) {
    unsigned char data[8];
    int arr_ind = 0, data_ind = 0;

    usb_write(pedal->header);
    memset(data, 0, 8);
    while (arr_ind < pedal->data_len) {
        if (data_ind == 8) {
            usb_write(data);
            memset(data, 0, 8);
            data_ind = 0;
        }
        data[data_ind++] = pedal->data[arr_ind++];
    }
    usb_write(data);
}

void write_pedals() {
    /*
    int i = 0;
    printf("start: ");
    debug_arr(pd.start, 8);
    for (i = 0 ; i < 3 ; i++) {
        printf("pedal %d header: ", i+1);
        debug_arr(pd.pedals[i].header, 8);
        printf("pedal %d data: ", i+1);
        debug_arr(pd.pedals[i].data, pd.pedals[i].data_len);
    }
    */
    usb_write(pd.start);
    write_pedal(&pd.pedals[0]);
    write_pedal(&pd.pedals[1]);
    write_pedal(&pd.pedals[2]);
}

int main(int argc, char *argv[]) {
    int opt;

    if (argc == 1) {
        usage(argv[0]);
    }
    if (argc == 2 && strcmp(argv[1], "-r") == 0) {
        init();
        read_pedals();
        deinit();
        return 0;
    }
    init_pedals();
    while ((opt = getopt(argc, argv, "123rs:S:a:k:m:b:x:y:w:")) != -1) {
        switch (opt) {
            case '1':
                curr_pedal = &pd.pedals[0];
                break;
            case '2':
                curr_pedal = &pd.pedals[1];
                break;
            case '3':
                curr_pedal = &pd.pedals[2];
                break;
            case 'r':
                fprintf(stderr, "Cannot use -r with other options\n");
                return 1;
            case 's':
                compile_string(optarg);
                break;
            case 'S':
                compile_raw_string(optarg);
                break;
            case 'a':
                compile_string_key(optarg);
                break;
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

