#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <hidapi.h>
#include <stdint.h>
#include "common.h"
#include "debug.h"

hid_device *dev = NULL;

static const int template_len = 0x1a;
static uint8_t template[0x1a] = {
    0x1a, 0x00, 0x01, 0x10, 0xf0, 0x05, 0x01, 0x10,
    0xf0, 0x1f, 0x01, 0x10, 0xf0, 0x20, 0x01, 0x10,
    0xf0, 0x21, 0x01, 0x10, 0xf0, 0x22, 0x01, 0x10,
    0xf0, 0x23};

void usage()
{
    fprintf(stderr, "Usage: scythe [-123] [-r] [-a <key>] [-m <modifier>] [-b <button>]\n"
        "   -r          - read all pedals\n"
        "   -1          - program the first pedal\n"
        "   -2          - program the second pedal (default)\n"
        "   -3          - program the third pedal\n"
        "   -a key      - append the specified key\n"
        "   -m modifier - ctrl|shift|alt|win\n"
        "   -b button   - mouse_left|mouse_middle|mouse_right|mouse_double\n\n"
        "You cannot mix -a and -m options with -b option for one and the same pedal\n");
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
void prepare(uint8_t *data, int len)
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
    prepare(data, len);
    send_report(data, len);
}

// BXKBSettingLib.dll + 0x1540
void UpdateSetting(uint8_t *data, int len)
{
    uint8_t buff[0x48] = {0};
    buff[3] = 0x2c;         // local_45
    buff[6] = 0x02;         // local_42
    SetUpdateEx(buff, 0x48);
    int offset = 0;         // iVar5
    int count = 0x20;       // uVar4
    if (len < 0x20) {
        count = len;
    }
    len = len - count;
    buff[3] = 0x26;             // local_45
    buff[4] = offset >> 8;       // local_44
    buff[5] = offset;            // local_43
    buff[6] = count;            // local_42
    for (int i = 0; i < count; i++) {
        buff[8+i] = data[offset+i];
    }
    SetUpdateEx(buff, 0x48);
    SetUpdateEx(buff, 0x48);
    buff[3] = 0x2b;             // local_45
    buff[4] = 0x14;             // local_44
    buff[5] = 0x23;             // local_43
    buff[6] = 0x00;             // local_42
    SetUpdateEx(buff, 0x48);
}

void compile_key(const char *key)
{
    uint8_t b = 0;
    if (!encode_key(key, &b)) {
        fprintf(stderr, "Cannot encode key '%s'\n", key);
        exit(1);
    }
    template[5] = b;
}

void write_pedals() {
    uint8_t buff[0x48] = {0};
    SetUpdateEx(buff, 0x48);
    UpdateSetting(template, template_len);
    printf("Done. Unplug the footswitch and then plug it back again.\n");
}

int main(int argc, char *argv[]) {
    int opt;

    if (argc == 1) {
        usage();
    }
    if (argc == 2 && strcmp(argv[1], "-r") == 0) {
        init();
        // TODO: implement
        // read_pedals();
        deinit();
        return 0;
    }
    while ((opt = getopt(argc, argv, "123ra:m:b:")) != -1) {
        switch (opt) {
            case '1':
                // TODO: implement
                break;
            case '2':
                // TODO: implement
                break;
            case '3':
                // TODO: implement
                break;
            case 'r':
                fprintf(stderr, "Cannot use -r with other options\n");
                return 1;
            case 'a':
                compile_key(optarg);
                break;
            case 'm':
                // TODO: implement
                //compile_modifier(optarg);
                break;
            case 'b':
                // TODO: implement
                //compile_mouse_button(optarg);
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
