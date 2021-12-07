#include <libusb-1.0/libusb.h>
#include <stdio.h>

unsigned char endpoint_in = 0x81;
unsigned char endpoint_out = 0x02;

libusb_device_handle *device;
int initialized = 0;

typedef struct gc_inputs {
    unsigned char status;
    union {
        char btn_l;
        struct {
            int a        : 1;
            int b        : 1;
            int x        : 1;
            int y        : 1;
            int dleft    : 1;
            int dright   : 1;
            int ddown    : 1;
            int dup      : 1;
        };
    };
    union {
        char btn_h;
        struct {
            int start    : 1;
            int z        : 1;
            int r        : 1;
            int l        : 1;
        };
    };
    char ax;
    char ay;
    char cx;
    char cy;
    unsigned char lt;
    unsigned char rt;
} gc_inputs;

static void error(const char msg[])
{
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}

static void debug_print_hex(unsigned char data[], int len)
{
    char msg[1024];
    char buf[16];

    msg[0] = '\0';
    buf[0] = '\0';

    for (int i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02X ", data[i]);
        strcat(msg, buf);
    }

    MessageBoxA(NULL, msg, "Debug", MB_OK);
}

void gc_init()
{
    if (initialized) return;

    int err;

    err = libusb_init(NULL);
    if (err) {
        error("Failed to initialize libusb");
        return;
    }

    // open first available device
    device = libusb_open_device_with_vid_pid(NULL, 0x057E, 0x0337);
    if (!device) {
        error("Failed to open adapter");
        return;
    }

    // nyko fix
    const int transfer = libusb_control_transfer(device, 0x21, 11, 0x0001, 0, NULL, 0, 1000);

    err = libusb_claim_interface(device, 0);
    if (err) {
        error(libusb_error_name(err));
        return;
    }

    // begin polling
    unsigned char cmd = 0x13;
    unsigned char readbuf[37];
    int transferred = 0;

    err = libusb_interrupt_transfer(
        device, endpoint_out, &cmd, sizeof(cmd), &transferred, 16
    );
    if (err) {
        error(libusb_error_name(err));
    }

    err = libusb_interrupt_transfer(
        device, endpoint_in, readbuf, sizeof(readbuf), &transferred, 16
    );
    if (err) {
        error(libusb_error_name(err));
    }

    initialized = 1;
}

void gc_deinit()
{
    if (!initialized) return;

    if (device) {
        libusb_release_interface(device, 0);
        libusb_close(device);
    }

    libusb_exit(NULL);

    initialized = 0;
}

int gc_get_inputs(gc_inputs *i)
{
    if (!initialized) return -1;

    unsigned char readbuf[37];
    int transferred = 0;

    int err = libusb_interrupt_transfer(
        device, endpoint_in, readbuf, sizeof(readbuf), &transferred, 16
    );
    if (err) {
        error(libusb_error_name(err));
        gc_deinit();
        return -2;
    }

    i->status = readbuf[1];
    i->btn_l  = readbuf[2];
    i->btn_h  = readbuf[3];
    i->ax     = readbuf[4]-128; // TODO: proper calibration
    i->ay     = readbuf[5]-128;
    i->cx     = readbuf[6]-128;
    i->cy     = readbuf[7]-128;
    i->lt     = readbuf[8];
    i->rt     = readbuf[9];

    return 0;
}
