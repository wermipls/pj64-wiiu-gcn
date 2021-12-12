#include <stdio.h>

struct config cfg;

static const char configpath[] = "./Config/" PLUGIN_NAME ".bin";

enum XY_mode
{
    XY_CBUTTONS,
    XY_L_4CBUTTONS,
};

struct config 
{
    int swap_zl;
    int analog_trig;
    int range;
    int trig_thres;
    int cstick_thres;
    int dz;
    enum XY_mode xy_mode;
    int zl_as_z;
    int async;
};

void config_defaults()
{
    cfg.swap_zl = 1;
    cfg.analog_trig = 0;
    cfg.zl_as_z = 0;
    cfg.range = 80;
    cfg.trig_thres = 128;
    cfg.cstick_thres = 64;
    cfg.dz = 0;
    cfg.xy_mode = XY_CBUTTONS;
    cfg.async = 1;
}

void config_load()
{
    config_defaults();

    FILE *f = fopen(configpath, "rb");

    fread(&cfg, sizeof(cfg), 1, f);

    fclose(f);
}

void config_save()
{
    CreateDirectory("Config", NULL);
    FILE *f = fopen(configpath, "wb");

    fwrite(&cfg, sizeof(cfg), 1, f);

    fclose(f);
}
