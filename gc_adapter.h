#pragma once

#include <Windows.h>

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
    int ax;
    int ay;
    int cx;
    int cy;
    unsigned char lt;
    unsigned char rt;
    int ax_rest;
    int ay_rest;
    int cx_rest;
    int cy_rest;
    unsigned char lt_rest;
    unsigned char rt_rest;

} gc_inputs;

extern CRITICAL_SECTION gc_critical;

/* attempts to initialize the adapter.
 * if async_mode is non-zero, a polling thread will be started */
void gc_init(int async_mode);

void gc_deinit();
int gc_is_present(int status);

/* polls the adapter and fills out the internal gc_inputs array
 * returns 0 on success */
int gc_poll_inputs();

/* fills out a gc_inputs struct with the inputs from a specified controller.
 * returns 0 on success */
int gc_get_inputs(int index, gc_inputs *inputs);

/* fills out a gc_inputs array with the inputs from a specified controller.
 * returns 0 on success */
int gc_get_all_inputs(gc_inputs inputs[4]);

int gc_is_async();
float gc_test_pollrate();
