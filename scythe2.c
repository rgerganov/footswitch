#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <hidapi.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"
#include "debug.h"

#define MAX_KEYS 255

hid_device *dev = NULL;

enum event_type {
    NONE = 0,
    SINGLE_KEY_REPEAT = 0x10,
    SINGLE_KEY_NOREPEAT = 0x20,
    MULTIPLE_KEYS = 0x30,
} event_type;

typedef struct key_data {
    uint8_t mod;
    uint8_t code;
} key_data;

typedef struct pedal_data
{
    enum event_type type;
    int count;
    key_data keys[MAX_KEYS];
} pedal_data;

pedal_data pedals[6];
int curr_pedal = 0;

void usage()
{
    fprintf(stderr, "Usage: scythe2 [-123456] [-r] [-k <key>] [-a <key>] [-m <modifier>] [-b <button>]\n"
        "   -r          - read all pedals\n"
        "   -1          - program the first pedal\n"
        "   -2          - program the second pedal (default)\n"
        "   -3          - program the third pedal\n"
        "   -4          - program the fourth pedal\n"
        "   -5          - program the fifth pedal\n"
        "   -6          - program the sixth pedal\n"
        "   -s string   - append the specified string\n"
        "   -a key      - write the specified key (no repeat)\n"
        "   -k key      - write the specified key (repeat)\n"
        "   -m modifier - ctrl|shift|alt|win\n"
        "   -b button   - mouse_left|mouse_middle|mouse_right|mouse_double\n");
    exit(1);
}

void init()
{
    hid_init();
    dev = hid_open(0x055a, 0x0998, NULL);
    if (dev == NULL) {
        fatal("Cannot find Scythe pedal with VID:PID=055a:0998.\nCheck that a Scythe device is connected and that you have the correct permissions to access it.");
    }
}

void deinit()
{
    hid_close(dev);
    hid_exit();
}

void send_report(unsigned char *ptr, int len)
{
    int r;
    //debug_arr(ptr, len);
    r = hid_send_feature_report(dev, ptr, len);
    if (r < 0) {
        fprintf(stderr, "Error sending feature report\n");
    }
    usleep(200 * 1000);
}

// BXKBSettingLib.dll + 0x10B0
void checksum(uint8_t *data, int len)
{
    uint8_t sum = 0;
    data[7] = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    data[7] = sum;
}

// BXKBSettingLib.dll + 0x10E0
void SetUpdateEx(uint8_t *data, int len)
{
    data[0] = 0x05;
    data[1] = 0x96;
    data[2] = 0xa5;
    checksum(data, len);
    send_report(data, len);
}

// BXKBSettingLib.dll + 0x1540
void UpdateSetting(uint8_t *data, int len)
{
    uint8_t buff[0x48] = {0};
    buff[3] = 0x2c;         // local_45
    buff[6] = 0x02;         // local_42
    SetUpdateEx(buff, 0x48);
    for (int offset = 0; offset < len; offset += 0x20) {
        int count = len - offset;
        if (count > 0x20) {
            count = 0x20;
        }
        buff[3] = 0x26;             // local_45
        buff[4] = offset >> 8;       // local_44
        buff[5] = offset;            // local_43
        buff[6] = count;            // local_42
        for (int i = 0; i < count; i++) {
            buff[8+i] = data[offset+i];
        }
        SetUpdateEx(buff, 0x48);
        SetUpdateEx(buff, 0x48);
    }
    buff[3] = 0x2b;             // local_45
    buff[4] = 0x14;             // local_44
    buff[5] = 0x23;             // local_43
    buff[6] = 0x00;             // local_42
    SetUpdateEx(buff, 0x48);
}

static bool set_pedal_type(enum event_type new_type)
{
    if (pedals[curr_pedal].type == NONE) {
        pedals[curr_pedal].type = new_type;
        return true;
    }
    return false;
}

void compile_string(const char *str)
{
    if (!set_pedal_type(MULTIPLE_KEYS)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    int len = strlen(str);
    if (len > MAX_KEYS) {
        fprintf(stderr, "The string length exceeds %d\n", MAX_KEYS);
        exit(1);
    }
    pedals[curr_pedal].count = len;
    for (int i = 0; i < strlen(str); i++) {
        pedals[curr_pedal].keys[i].mod = 0xf0;
        uint8_t code = 0;
        encode_char(str[i], &code);
        pedals[curr_pedal].keys[i].code = code;
    }
}

void compile_key_norepeat(const char *key)
{
    if (!set_pedal_type(SINGLE_KEY_NOREPEAT)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    uint8_t code = 0;
    if (!encode_key(key, &code)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }
    pedals[curr_pedal].count = 1;
    pedals[curr_pedal].keys[0].mod |= 0xf0;
    pedals[curr_pedal].keys[0].code = code;
}

void compile_key_repeat(const char *key)
{
    if (!set_pedal_type(SINGLE_KEY_REPEAT)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    uint8_t code = 0;
    if (!encode_key(key, &code)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }
    pedals[curr_pedal].count = 1;
    pedals[curr_pedal].keys[0].mod |= 0xf0;
    pedals[curr_pedal].keys[0].code = code;
}

void compile_modifier(const char *mod_str)
{
    enum modifier mod;

    if (!parse_modifier(mod_str, &mod)) {
        fprintf(stderr, "Invalid modifier '%s'\n", mod_str);
        exit(1);
    }
    pedals[curr_pedal].keys[0].mod |= mod;
}

void compile_mouse_button(const char *btn_str)
{
    enum mouse_button btn;
    if (!set_pedal_type(SINGLE_KEY_REPEAT)) {
        fprintf(stderr, "Invalid combination of options\n");
        usage();
    }
    if (!parse_mouse_button(btn_str, &btn)) {
        fprintf(stderr, "Invalid mouse button '%s'\n", btn_str);
        exit(1);
    }
    pedals[curr_pedal].count = 1;
    pedals[curr_pedal].keys[0].mod = 0xc0;
    pedals[curr_pedal].keys[0].code = btn;
}

void write_pedals()
{
    int data_length = 2;
    for (int i = 0; i < 6; i++) {
        // if the user hasn't specified anything, program the pedal with key 'a'
        if (pedals[i].type == NONE) {
            pedals[i].type = SINGLE_KEY_REPEAT;
            pedals[i].count = 1;
            pedals[i].keys[0].mod = 0xf0;
            pedals[i].keys[0].code = 4;
        }
        data_length += pedals[i].count*2 + 2;
    }
    uint8_t *data = malloc(data_length);
    if (!data) {
        fprintf(stderr, "Not enough memory\n");
        exit(1);
    }

    // <len % 256> <len / 256>
    // <count> <type> <mod> <code> [<mod> <code>]
    // <count> <type> <mod> <code> [<mod> <code>]
    // <count> <type> <mod> <code> [<mod> <code>]
    // <count> <type> <mod> <code> [<mod> <code>]
    // <count> <type> <mod> <code> [<mod> <code>]
    // <count> <type> <mod> <code> [<mod> <code>]
    //
    // Note: count>1 only if type==0x30
    // Note: mod==0xc0 for mouse buttons
    int ind = 0;
    data[ind++] = data_length % 256;
    data[ind++] = data_length / 256;
    for (int i = 0; i < 6; i++) {
        data[ind++] = pedals[i].count;
        data[ind++] = pedals[i].type;
        for (int j = 0; j < pedals[i].count; j++) {
            data[ind++] = pedals[i].keys[j].mod;
            data[ind++] = pedals[i].keys[j].code;
        }
    }

    // printf("data_length = %d\n", data_length);
    // for (int i = 0; i < data_length; i++) {
    //     printf("%02X ", data[i]);
    // }
    // printf("\n");

    uint8_t buff[0x48] = {0};
    SetUpdateEx(buff, 0x48);
    UpdateSetting(data, data_length);
    printf("Done. Unplug the footswitch and then plug it back again.\n");
    free(data);
}

static void print_key(int mod, uint8_t code)
{
    if (mod == 0xc0) {
        if (code & MOUSE_LEFT) {
            printf("mouse left\n");
        }
        if (code & MOUSE_RIGHT) {
            printf("mouse right\n");
        }
    } else {
        if (mod & CTRL) {
            printf("ctrl+");
        }
        if (mod & SHIFT) {
            printf("shift+");
        }
        if (mod & ALT) {
            printf("alt+");
        }
        if (mod & WIN) {
            printf("win+");
        }
        printf("%s\n", decode_byte(code));
    }
}

static void print_pedal(int num, const uint8_t *data)
{
    int count = data[0];
    int type = data[1];
    switch (type) {
    case SINGLE_KEY_REPEAT:
        printf("Pedal %d (single key repeat): ", num);
        print_key(data[2], data[3]);
        break;
    case SINGLE_KEY_NOREPEAT:
        printf("Pedal %d (single key no repeat): ", num);
        print_key(data[2], data[3]);
        break;
    case MULTIPLE_KEYS:
        printf("Pedal %d (multiple keys): ", num);
        for (int i = 0; i < count; i++) {
            printf("%s", decode_byte(data[3+i*2]));
        }
        printf("\n");
        break;
    }
}

static void read_pedals()
{
    uint8_t buff[0x48] = {0};
    buff[3] = 0x5a;
    SetUpdateEx(buff, 0x48);
    int r = hid_get_feature_report(dev, buff, 0x48);
    if (r < 0) {
        fatal("error getting feature report (%ls)", hid_error(dev));
    }
    int ind = 2;
    for (int i = 0; i < 6; i++) {
        int count = buff[ind];
        int length = count*2 + 2;
        if (ind + length > 0x48) {
            // TODO: find how to get the data after 0x48
            break;
        }
        print_pedal(i+1, buff+ind);
        ind += length;
    }
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
    while ((opt = getopt(argc, argv, "123456rs:a:k:m:b:")) != -1) {
        switch (opt) {
            case '1':
                curr_pedal = 0;
                break;
            case '2':
                curr_pedal = 1;
                break;
            case '3':
                curr_pedal = 2;
                break;
            case '4':
                curr_pedal = 3;
                break;
            case '5':
                curr_pedal = 4;
                break;
            case '6':
                curr_pedal = 5;
                break;
            case 'r':
                fprintf(stderr, "Cannot use -r with other options\n");
                return 1;
            case 's':
                compile_string(optarg);
                break;
            case 'a':
                compile_key_norepeat(optarg);
                break;
            case 'k':
                compile_key_repeat(optarg);
                break;
            case 'm':
                compile_modifier(optarg);
                break;
            case 'b':
                compile_mouse_button(optarg);
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
