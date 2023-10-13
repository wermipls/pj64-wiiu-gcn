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

// enum values are same as zilmar-spec, except decremented by 1
// (see zilmar_controller_1.0.h)
enum Accessory
{
    ACCESSORY_NONE = 0,
    ACCESSORY_CPAK = 1,
};

struct ConfigController
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

struct ConfigControllerEx
{
    struct _cfgctlex_fields
    {
    };

    // maintain constant struct size for backwards compatible expansion
    // without requiring version-specific checks.

    // truth is, i should have included reserved space in the other struct
    // instead of making a new one, but lol actually anticipating change lmao
    char _reserved[256 - sizeof(struct _cfgctlex_fields)];
};

_Static_assert(sizeof(struct ConfigControllerEx) == 256, "ConfigControllerEx struct is not 256 bytes");

struct Config 
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

    struct ConfigController controller[4];
    struct ConfigControllerEx controller_ex[4];
};

extern struct Config cfg;

void config_defaults();
void config_load();
void config_save();
