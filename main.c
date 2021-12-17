#define WIN32_LEAN_AND_MEAN

#define PLUGIN_NAME "pj64-wiiu-gcn"
#define PLUGIN_VERSION "0.2.0"
#define PLUGIN_NAMEVER PLUGIN_NAME " v" PLUGIN_VERSION
#define PLUGIN_REPO "https://github.com/wermipls/pj64-wiiu-gcn"

#include <Windows.h>
#include <Shlwapi.h>
#include "zilmar_controller_1.0.h"
#include "gc_adapter.h"
#include "config.h"
#include "gui.h"
#include "util.h"

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        hInstance = hinstDLL;
        log_open();
        config_load();
        InitializeCriticalSection(&gc_critical);
        break;
    case DLL_PROCESS_DETACH:
        log_close();
        DeleteCriticalSection(&gc_critical);
        break;
    }
    return TRUE;
}

EXPORT void CALL CloseDLL(void)
{
	gc_deinit();
}

EXPORT void CALL ControllerCommand(int Control, BYTE *Command)
{
    // this func is apparently useless, because ReadController
    // is called right after and receives the exact same data. lol 
    return;
}

EXPORT void CALL DllAbout(HWND hParent)
{
	MessageBox(
        hParent, 
        "Proof of concept Wii U Gamecube adapter plugin\n"
        "Version " PLUGIN_VERSION "\n"
        "Compiled on " __DATE__ "\n\n"
        PLUGIN_REPO,
        "About " PLUGIN_NAMEVER,
        MB_OK | MB_ICONINFORMATION
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
    gc_inputs i;

    int err = gc_get_inputs(Control, &i);
    if (err)
        return;

    Keys->Value = 0;

    if (!gc_is_present(i.status)) {
        return;
    }

    Keys->X_AXIS = deadzone(i.ay, cfg.dz) * cfg.range / 100;
    Keys->Y_AXIS = deadzone(i.ax, cfg.dz) * cfg.range / 100;

    Keys->A_BUTTON = i.a;
    Keys->B_BUTTON = i.b;

    int lt = cfg.analog_trig ? i.lt > cfg.trig_thres
                             : 0;
    int rt = cfg.analog_trig ? i.rt > cfg.trig_thres
                             : 0;

    if (!cfg.zl_as_z) {
        if (cfg.swap_zl) {
            Keys->Z_TRIG = i.l || lt;
            Keys->L_TRIG = i.z;
        } else {
            Keys->Z_TRIG = i.z;
            Keys->L_TRIG = i.l || lt;
        } 
    } else {
        Keys->Z_TRIG = i.z || i.l || lt;
    }

    Keys->R_TRIG = i.r || rt;
    Keys->START_BUTTON = i.start;

    Keys->L_DPAD = i.dleft;
    Keys->R_DPAD = i.dright;
    Keys->U_DPAD = i.dup;
    Keys->D_DPAD = i.ddown;

    struct Vec2 cstick = circle_to_square(i.cx, i.cy);

    Keys->L_CBUTTON = cstick.x < -cfg.cstick_thres;
    Keys->R_CBUTTON = cstick.x >  cfg.cstick_thres;
    Keys->D_CBUTTON = cstick.y < -cfg.cstick_thres;
    Keys->U_CBUTTON = cstick.y >  cfg.cstick_thres;

    if (cfg.xy_mode == XY_CBUTTONS) {
        Keys->L_CBUTTON |= i.y;
        Keys->R_CBUTTON |= i.x;
    } else {
        Keys->L_TRIG |= i.y;

        Keys->L_CBUTTON |= i.x;
        Keys->R_CBUTTON |= i.x;
        Keys->D_CBUTTON |= i.x;
        Keys->U_CBUTTON |= i.x;
    }
}

EXPORT void CALL InitiateControllers(HWND hMainWindow, CONTROL Controls[4])
{
    dlog(LOG_INFO, "InitiateControllers()");
    gc_init(cfg.async);

    gc_inputs gc[4];

    // workaround for incorrect status being reported
    // when trying to use the adapter directly after it's been plugged in.
    // i < 2 works for my official adapter, but i've put in i < 10 to be safe
    // (which should stall for only 80ms with the default 125hz pollrate)
    if (!gc_is_async()) { 
        for (int i = 0; i < 10; ++i) { 
            int err = gc_get_all_inputs(gc);
            if (err) return;
        }
    } else {
        Sleep(80);
        int err = gc_get_all_inputs(gc);
        if (err) return;
    }

    int concount = 0;

    for (int i = 0; i < 4; ++i) {
        int status = gc[i].status;

        if (gc_is_present(status)) {
            Controls[i].Present = TRUE;
            dlog(LOG_INFO, "Controller %d present, status 0x%X", i, status);
            ++concount;
        } else {
            Controls[i].Present = FALSE;
            dlog(LOG_INFO, "Controller %d not available, status 0x%X", i, status);
        }

        Controls[i].RawData = TRUE;
        Controls[i].Plugin = PLUGIN_RAW;
    }

    if (concount == 0) {
        MessageBox(
            hMainWindow, 
            "No controllers detected.\n\n"
            "Please plug in a controller, then restart the emulator.",
            PLUGIN_NAME " info", MB_OK | MB_ICONINFORMATION
        );
    }
}

EXPORT void CALL ReadController(int Control, BYTE *Command)
{
    if (Control == -1)
        return; // why make a call in the 1st place if there's nothing to do?

    // from what i understand:
    // Command[0] is command length. in practice, the command is always 1 byte
    // Command[1] is length of our response in bytes
    // so the actual data (command then response) only begins at Command[2]

    unsigned char *len_cmd  = &Command[0];
    unsigned char *len_data = &Command[1];
    unsigned char *cmd      = &Command[2];
    unsigned char *data     = &Command[2 + *len_cmd];

    dlog(LOG_INFO, "Controller command: 0x%X", *cmd);

    switch (*cmd)
    {
    case 0xFF: // controller info/reset
    case 0x00: // controller info
        data[0] = 0x05; // normal n64 controller
        data[1] = 0x00; // no controller pak
        break;
    case 0x01: // controller status
        GetKeys(Control, (BUTTONS*)data); // HACK: this would optimally be a separate func
        break;
    default:
        *len_data = *len_data | 0b10000000;
        break; 
    }
}

EXPORT void CALL RomClosed(void)
{
    // something in here
}

EXPORT void CALL RomOpen(void)
{
    gc_init(cfg.async);
}
