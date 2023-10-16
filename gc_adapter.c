#include "gc_adapter.h"
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <synchapi.h>
#include <stdatomic.h>
#include <time.h>
#include "log.h"
#include "util.h"

unsigned char endpoint_in = 0x81;
unsigned char endpoint_out = 0x02;

libusb_device_handle *device;
static atomic_int initialized = 0;
static atomic_int pending_deinit = 0;
static atomic_int is_async = 0;
static _Atomic enum GCError init_error = GCERR_NOT_INITIALIZED;

HANDLE poll_thread;
HANDLE terminate_event;
CRITICAL_SECTION gc_critical;

HANDLE polltest_start;
HANDLE polltest_end;
atomic_int poll_count = 0;

static atomic_int rumble_enabled[4] = {0};

static gc_inputs gc[4];

DWORD WINAPI gc_polling_thread(LPVOID param);

void gc_init(int async_mode)
{
    dlog(LOG_INFO, "gc_init()");
    if (initialized) return;
    dlog(LOG_INFO, "Attempting to initialize the adapter");

    int err;

    err = libusb_init(NULL);
    if (err) {
        dlog(LOG_ERR, "Failed to initialize libusb");
        init_error = GCERR_LIBUSB_INIT; 
        return;
    }

    // open first available device
    device = libusb_open_device_with_vid_pid(NULL, 0x057E, 0x0337);
    if (!device) {
        dlog(LOG_ERR_NO_MSGBOX, "Failed to open adapter");
        init_error = GCERR_LIBUSB_OPEN;
        return;
    }

    // nyko fix
    libusb_control_transfer(device, 0x21, 11, 0x0001, 0, NULL, 0, 1000);

    err = libusb_claim_interface(device, 0);
    if (err) {
        dlog(LOG_ERR, "Failed to claim interface, %s", libusb_error_name(err));
        init_error = GCERR_LIBUSB_CLAIM_INTERFACE;
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
        dlog(LOG_ERR, "Failed out transfer, %s", libusb_error_name(err));
    }

    err = libusb_interrupt_transfer(
        device, endpoint_in, readbuf, sizeof(readbuf), &transferred, 16
    );
    if (err) {
        dlog(LOG_ERR, "Failed in transfer, %s", libusb_error_name(err));
    }

    if (async_mode) {
        // start a thread
        dlog(LOG_INFO, "Starting a polling thread");
        poll_thread = CreateThread(NULL, 0, gc_polling_thread, NULL, 0, NULL); 
        if (!poll_thread) {
            dlog(LOG_ERR, "Failed to create a polling thread");
            init_error = GCERR_CREATE_THREAD;
        }
        is_async = 1;
    } else {
        is_async = 0;
    }

    initialized = 1;
    init_error = GCERR_OK;
}

enum GCError gc_get_init_error()
{
    return init_error;
}

static void gc_handle_rumble()
{
    if (!is_async) {
        uint8_t cmd[5] = {
            0x11,
            rumble_enabled[0],
            rumble_enabled[1],
            rumble_enabled[2],
            rumble_enabled[3],
        };

        int transferred = 0;
        int err = libusb_interrupt_transfer(
            device, endpoint_out, cmd, sizeof(cmd), &transferred, 16
        );
        if (err) {
            dlog(LOG_WARN, "Failed out transfer, %s", libusb_error_name(err));
        }
    }
}

void gc_deinit()
{
    dlog(LOG_INFO, "gc_deinit()");
    if (!initialized) return;

    if (is_async) {
        dlog(LOG_INFO, "Terminating the polling thread");
        terminate_event = CreateEvent(NULL, FALSE, TRUE, NULL);
        WaitForSingleObject(poll_thread, INFINITE);
        dlog(LOG_INFO, "...done");
    }

    rumble_enabled[0] = 0;
    rumble_enabled[1] = 0;
    rumble_enabled[2] = 0;
    rumble_enabled[3] = 0;

    if (device) {
        gc_handle_rumble();

        dlog(LOG_INFO, "Closing the adapter");
        libusb_release_interface(device, 0);
        libusb_close(device);
    }

    libusb_exit(NULL);

    initialized = 0;
    init_error = GCERR_NOT_INITIALIZED;
}

void handle_pending_deinit()
{
    if (pending_deinit) {
        gc_deinit();
        pending_deinit = 0;
    }
}

int gc_is_present(int status)
{
    return status;
}

int gc_poll_inputs()
{
    if (!initialized || pending_deinit) return -1;

    unsigned char readbuf[37];
    int transferred = 0;

    int err = libusb_interrupt_transfer(
        device, endpoint_in, readbuf, sizeof(readbuf), &transferred, 16
    );
    if (err) {
        if (err == LIBUSB_ERROR_TIMEOUT) {
            dlog(LOG_WARN, "Failed in transfer, %s", libusb_error_name(err));
        } else {
            dlog(LOG_ERR_NO_MSGBOX, "Failed in transfer, %s", libusb_error_name(err));
            pending_deinit = 1;
        }
        return -2;
    }
    if (transferred != 37) {
        dlog(LOG_WARN, "Expected %d bytes response, got %d", 37, transferred);
    }

    EnterCriticalSection(&gc_critical);

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
        if (!gc_is_present(gc[i].status_old) && gc_is_present(gc[i].status)) {
            // heuristic to avoid a recalib bug happening with oc'd adapter
            if (gc[i].ax | gc[i].ay | gc[i].cx | gc[i].cy | gc[i].lt | gc[i].rt) {
                dlog(LOG_INFO, "Controller %d plugged in, calibrating centers", i);
                gc[i].ax_rest = gc[i].ax;
                gc[i].ay_rest = gc[i].ay;
                gc[i].cx_rest = gc[i].cx;
                gc[i].cy_rest = gc[i].cy;
                gc[i].lt_rest = gc[i].lt;
                gc[i].rt_rest = gc[i].rt;
            } else {
                gc[i].status = 0; // recalib next time
            }
        }

        // remove offsets
        gc[i].ax -= gc[i].ax_rest;
        gc[i].ay -= gc[i].ay_rest;
        gc[i].cx -= gc[i].cx_rest;
        gc[i].cy -= gc[i].cy_rest;

        gc[i].lt = smax((int)gc[i].lt - gc[i].lt_rest, 0);
        gc[i].rt = smax((int)gc[i].rt - gc[i].rt_rest, 0);
    }

    LeaveCriticalSection(&gc_critical);

    return 0;
}

DWORD WINAPI gc_polling_thread(LPVOID param)
{
    struct libusb_transfer *rumble = libusb_alloc_transfer(0);
    uint8_t rumble_cmd[] = { 0x11, 0, 0, 0, 0 };
    libusb_fill_interrupt_transfer(
        rumble, device, endpoint_out, rumble_cmd, sizeof(rumble_cmd), NULL, NULL, 16);

    while (WaitForSingleObject(terminate_event, 0)) {
        rumble_cmd[1] = rumble_enabled[0];
        rumble_cmd[2] = rumble_enabled[1];
        rumble_cmd[3] = rumble_enabled[2];
        rumble_cmd[4] = rumble_enabled[3];
        libusb_submit_transfer(rumble);

        gc_poll_inputs();
        ++poll_count;
    }

    rumble_cmd[1] = 0;
    rumble_cmd[2] = 0;
    rumble_cmd[3] = 0;
    rumble_cmd[4] = 0;

    libusb_interrupt_transfer(
        device, endpoint_out, rumble_cmd, sizeof(rumble_cmd), NULL, 100
    );

    libusb_free_transfer(rumble);

    return 0;
}

int gc_get_inputs(int index, gc_inputs *inputs)
{
    handle_pending_deinit();
    if (!initialized) return -1;

    if (!is_async) {
        // HACK: get the inputs only on p1 request to avoid 
        // needlessly waiting for 4 reports and stalling the emulator.
        int err = 0;
        if (index == 0)
            err = gc_poll_inputs();
        if (err)
            return -3;
    }

    EnterCriticalSection(&gc_critical);

    *inputs = gc[index];

    LeaveCriticalSection(&gc_critical);

    return 0;
}

int gc_get_all_inputs(gc_inputs inputs[4])
{
    handle_pending_deinit();
    if (!initialized) return -1;

    if (!is_async) {
        int err = gc_poll_inputs();
        if (err)
            return -3;
    }

    EnterCriticalSection(&gc_critical);

    inputs[0] = gc[0];
    inputs[1] = gc[1];
    inputs[2] = gc[2];
    inputs[3] = gc[3];

    LeaveCriticalSection(&gc_critical);

    return 0;
}

int gc_set_rumble(int index, int enabled)
{
    handle_pending_deinit();
    if (!initialized) return -1;

    if (index < 0 || index >= 4)
        return -2;

    rumble_enabled[index] = !(!enabled);

    gc_handle_rumble();

    return 0;
}

int gc_reset_rumble()
{
    handle_pending_deinit();
    if (!initialized) return -1;

    rumble_enabled[0] = 0;
    rumble_enabled[1] = 0;
    rumble_enabled[2] = 0;
    rumble_enabled[3] = 0;

    gc_handle_rumble();

    return 0;
}

int gc_is_async()
{
    return is_async;
}

float gc_test_pollrate()
{
    if (!is_async || !initialized)
        return -1;

    poll_count = 0;
    
    struct timeval old;
    gettimeofday(&old, NULL);
    
    Sleep(1000);

    struct timeval new;
    gettimeofday(&new, NULL);

    struct timeval delta;
    delta.tv_sec = new.tv_sec - old.tv_sec;
    delta.tv_usec = new.tv_usec - old.tv_usec;

    float delta_s = delta.tv_sec + (float)delta.tv_usec / 1000000;
    
    return poll_count / delta_s;
}
