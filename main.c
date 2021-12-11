#define WIN32_LEAN_AND_MEAN

#define PLUGIN_NAME "pj64-wiiu-gcn"
#define PLUGIN_VERSION "0.1.0"
#define PLUGIN_NAMEVER PLUGIN_NAME " v" PLUGIN_VERSION
#define PLUGIN_REPO "https://github.com/wermipls/pj64-wiiu-gcn"

#include <Windows.h>
#include <Shlwapi.h>
#include "zilmar_controller_1.0.h"
#include "adapter.h"
#include "config.h"
#include "gui.h"
#include "util.h"

gc_inputs gamecube[4];
HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        hInstance = hinstDLL;
        log_open();
        config_load();
        break;
    case DLL_PROCESS_DETACH:
        log_close();
        break;
    }
    return TRUE;
}

EXPORT void CALL CloseDLL(void)
{
	gc_deinit();
}

EXPORT void CALL DllAbout(HWND hParent)
{
	MessageBoxA(
        hParent, 
        "Proof of concept Wii U Gamecube adapter plugin\n"
        "Version " PLUGIN_VERSION "\n"
        "Compiled on " __DATE__ "\n\n"
        PLUGIN_REPO,
        "About " PLUGIN_NAMEVER,
        MB_OK
    );
}

EXPORT void CALL DllConfig(HWND hParent)
{
    config_window(hInstance, hParent);
}

//EXPORT void CALL DllTest(HWND hParent);

EXPORT void CALL GetDllInfo(PLUGIN_INFO *PluginInfo)
{
    PluginInfo->Version = 0x0100;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
    strncpy(
        PluginInfo->Name,
        PLUGIN_NAMEVER,
        sizeof(PluginInfo->Name)
    );
}

EXPORT void CALL GetKeys(int Control, BUTTONS *Keys)
{
    int err = 0;

    // HACK: get the inputs only on p1 request to avoid 
    // needlessly waiting for 4 reports and stalling the emulator.
    // optimally the transfers would be done asynchronously
    if (Control == 0)
        err = gc_get_inputs(gamecube);
    if (err)
        return;

    gc_inputs *i = &gamecube[Control];
    Keys->Value = 0;

    if (i->status == GC_NOT_AVAILABLE) {
        return;
    }

    Keys->X_AXIS = deadzone(i->ay, cfg.dz) * cfg.range / 100;
    Keys->Y_AXIS = deadzone(i->ax, cfg.dz) * cfg.range / 100;

    Keys->A_BUTTON = i->a;
    Keys->B_BUTTON = i->b;

    int lt = cfg.analog_trig ? i->lt > cfg.trig_thres
                             : 0;
    int rt = cfg.analog_trig ? i->rt > cfg.trig_thres
                             : 0;

    if (cfg.swap_zl) {
        Keys->Z_TRIG = i->l || lt;
        Keys->L_TRIG = i->z;
    } else {
        Keys->Z_TRIG = i->z;
        Keys->L_TRIG = i->l || lt;
    }

    Keys->R_TRIG = i->r || rt;
    Keys->START_BUTTON = i->start;

    Keys->L_DPAD = i->dleft;
    Keys->R_DPAD = i->dright;
    Keys->U_DPAD = i->dup;
    Keys->D_DPAD = i->ddown;

    Keys->L_CBUTTON = i->cx < -cfg.cstick_thres;
    Keys->R_CBUTTON = i->cx >  cfg.cstick_thres;
    Keys->D_CBUTTON = i->cy < -cfg.cstick_thres;
    Keys->U_CBUTTON = i->cy >  cfg.cstick_thres;

    if (cfg.xy_mode == XY_CBUTTONS) {
        Keys->L_CBUTTON |= i->y;
        Keys->R_CBUTTON |= i->x;
    } else {
        Keys->L_TRIG |= i->y;

        Keys->L_CBUTTON |= i->x;
        Keys->R_CBUTTON |= i->x;
        Keys->D_CBUTTON |= i->x;
        Keys->U_CBUTTON |= i->x;
    }
}

EXPORT void CALL InitiateControllers(HWND hMainWindow, CONTROL Controls[4])
{
    dlog(LOG_INFO, "InitiateControllers()");
    gc_init();
    int err = gc_get_inputs(gamecube);
    if (err)
        return;

    for (int i = 0; i < 4; ++i) {
        switch (gamecube[i].status)
        {
            case GC_PRESENT:
                Controls[i].Present = TRUE;
                dlog(LOG_INFO, "Controller %d present", i);
                break;
            case GC_NOT_AVAILABLE:
                Controls[i].Present = FALSE;
                dlog(LOG_INFO, "Controller %d not available", i);
                break;
            default:
                Controls[i].Present = TRUE; // assuming its plugged in regardless
                dlog(LOG_WARN, "Unknown controller %d status: %d", i, gamecube[i].status);
        }
        Controls[i].Present = gamecube[i].status == GC_PRESENT;
        Controls[i].RawData = FALSE;
        Controls[i].Plugin = PLUGIN_NONE;
    }
}

//EXPORT void CALL ReadController(int Control, BYTE *Command);

EXPORT void CALL RomClosed(void)
{
    // something in here
}

EXPORT void CALL RomOpen(void)
{
    gc_init();
}
