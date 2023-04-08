#pragma once

enum MappingButtonAxis
{
    BA_NONE,
    BA_A,
    BA_B,
    BA_X,
    BA_Y,
    BA_DLEFT,
    BA_DRIGHT,
    BA_DDOWN,
    BA_DUP,
    BA_START,
    BA_Z,
    BA_R_BUTTON,
    BA_L_BUTTON,
    BA_R_ANALOG,
    BA_L_ANALOG,
    BA_LEFT,
    BA_RIGHT,
    BA_DOWN,
    BA_UP,
    BA_CLEFT,
    BA_CRIGHT,
    BA_CDOWN,
    BA_CUP,
    BA_MAX,
};

struct Mapping 
{
    enum MappingButtonAxis pri;
    enum MappingButtonAxis sec;
};

enum Accessory
{
    ACCESSORY_NONE,
    ACCESSORY_CPAK,
};

struct ConfigMapping
{
    int enabled;
    int force_plugged;
    enum Accessory accessory;
    struct Mapping a;
    struct Mapping b;
    struct Mapping z;
    struct Mapping l;
    struct Mapping r;
    struct Mapping start;
    struct Mapping d_left;
    struct Mapping d_right;
    struct Mapping d_up;
    struct Mapping d_down;
    struct Mapping c_left;
    struct Mapping c_right;
    struct Mapping c_up;
    struct Mapping c_down;
    struct Mapping analog_left;
    struct Mapping analog_right;
    struct Mapping analog_up;
    struct Mapping analog_down;
};

struct config 
{
    char header[4];
    unsigned int version;
    int range;
    int trig_thres;
    int stick_a2d_thres;
    int dz;
    int async;
    int scale_diagonals;
    int single_mapping;

    struct ConfigMapping mapping[4];
};

extern struct config cfg;

void config_defaults();
void config_load();
void config_save();
