#include <libusb-1.0/libusb.h>
#include <stdio.h>

unsigned char endpoint_in = 0x81;
unsigned char endpoint_out = 0x02;

libusb_device_handle *device;
int initialized = 0;

enum gc_status {
    GC_NOT_AVAILABLE = 0x00,
    GC_PRESENT = 0x10,
};

typedef struct gc_inputs {
    unsigned char status_old;
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
    char ax_rest;
    char ay_rest;
    char cx_rest;
    char cy_rest;
    unsigned char lt_rest;
    unsigned char rt_rest;

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
    libusb_control_transfer(device, 0x21, 11, 0x0001, 0, NULL, 0, 1000);

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

int gc_get_inputs(gc_inputs gc[])
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

    for (int i = 0; i < 4; ++i) {
        int offset = i * 9;
        gc[i].status_old = gc[i].status;

        gc[i].status = readbuf[offset+1];
        gc[i].btn_l  = readbuf[offset+2];
        gc[i].btn_h  = readbuf[offset+3];
        gc[i].ax     = readbuf[offset+4];
        gc[i].ay     = readbuf[offset+5];
        gc[i].cx     = readbuf[offset+6];
        gc[i].cy     = readbuf[offset+7];
        gc[i].lt     = readbuf[offset+8];
        gc[i].rt     = readbuf[offset+9];

        // calibrate centers if just plugged in
        if (gc[i].status_old == GC_NOT_AVAILABLE && gc[i].status == GC_PRESENT) {
            gc[i].ax_rest = gc[i].ax;
            gc[i].ay_rest = gc[i].ay;
            gc[i].cx_rest = gc[i].cx;
            gc[i].cy_rest = gc[i].cy;
            gc[i].lt_rest = gc[i].lt;
            gc[i].rt_rest = gc[i].rt;
        }

        // remove offsets
        gc[i].ax -= gc[i].ax_rest;
        gc[i].ay -= gc[i].ay_rest;
        gc[i].cx -= gc[i].cx_rest;
        gc[i].cy -= gc[i].cy_rest;

        // lets ignore trigger calib for now because
        // this is certainly not the right way to do it
        //gc[i].lt -= gc[i].lt_rest;
        //gc[i].rt -= gc[i].rt_rest;
    }

    return 0;
}
