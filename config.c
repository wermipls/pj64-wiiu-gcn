#include "config.h"
#include "log.h"
#include "plugin_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>
#include <shlwapi.h>

struct config cfg;

static char configpath[MAX_PATH];
int is_initialized = 0;

void config_init()
{
    if (is_initialized) return;

    HRESULT err = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, configpath);
    if (err)
    {
        dlog(LOG_ERR_NO_MSGBOX, "Failed to initialize config");
    }
    PathAppend(configpath, PLUGIN_NAME ".bin");

    is_initialized = 1;
}

void config_defaults()
{
    char header[4] = "PJGC";
    memcpy(cfg.header, header, sizeof(cfg.header));

    cfg.version = 0x0100;

    cfg.range = 80;
    cfg.trig_thres = 128;
    cfg.stick_a2d_thres = 64;
    cfg.dz = 0;
    cfg.async = 1;
    cfg.single_mapping = 0;
    cfg.scale_diagonals = 1;

    struct ConfigMapping m = {0};

    m.enabled = 1;
    m.accessory = ACCESSORY_NONE;

    m.a.pri = BA_A;
    m.b.pri = BA_B;
    m.start.pri = BA_START;
    m.l.pri = BA_Z;
    m.z.pri = BA_L_BUTTON;
    m.r.pri = BA_R_BUTTON;

    m.analog_up.pri    = BA_UP;
    m.analog_down.pri  = BA_DOWN;
    m.analog_left.pri  = BA_LEFT;
    m.analog_right.pri = BA_RIGHT;

    m.c_up.pri    = BA_CUP;
    m.c_down.pri  = BA_CDOWN;
    m.c_left.pri  = BA_CLEFT;
    m.c_right.pri = BA_CRIGHT;

    m.d_up.pri    = BA_DUP;
    m.d_down.pri  = BA_DDOWN;
    m.d_left.pri  = BA_DLEFT;
    m.d_right.pri = BA_DRIGHT;

    cfg.mapping[0] = m;
    cfg.mapping[1] = m;
    cfg.mapping[2] = m;
    cfg.mapping[3] = m;
}

void config_load()
{
    config_init();
    if (!is_initialized) return;

    config_defaults();

    FILE *f = fopen(configpath, "rb");
    if (f == NULL) {
        dlog(LOG_ERR_NO_MSGBOX, "Failed to load config file \"%s\": %s",
             configpath,
             strerror(errno));
        return;
    }

    struct config cfg_new;

    size_t bytes = fread(&cfg_new, sizeof(cfg_new), 1, f);
    if (bytes != sizeof(cfg_new)) {
        dlog(LOG_WARN, "Read %d bytes, config struct is %d",
             bytes, sizeof(cfg_new));
    }

    if (memcmp(cfg.header, cfg_new.header, sizeof(cfg.header))) {
        dlog(LOG_ERR_NO_MSGBOX, "Config file does not appear to be valid");
        fclose(f); 
        return;
    }

    if (cfg_new.version > cfg.version) {
        dlog(LOG_ERR_NO_MSGBOX, "Config file version is newer than current (%40x > %40x)",
             cfg_new.version > cfg.version);
        fclose(f); 
        return;
    }

    cfg = cfg_new;

    fclose(f);
}

void config_save()
{
    config_init();
    if (!is_initialized) return;

    FILE *f = fopen(configpath, "wb");

    if (f == NULL) {
        dlog(LOG_ERR, "Failed to save config file \"%s\": %s",
             configpath,
             strerror(errno));
        return;
    }

    fwrite(&cfg, sizeof(cfg), 1, f);

    fclose(f);
}

