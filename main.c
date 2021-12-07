#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include "zilmar_controller_1.0.h"
#include "adapter.h"

#define PLUGIN_NAME "pj64-wiiu-gcn"
#define PLUGIN_VERSION "0.0.1"
#define PLUGIN_NAMEVER PLUGIN_NAME " v" PLUGIN_VERSION

gc_inputs gamecube[4];

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
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
        "Proof of concept Wii U Gamecube input plugin\n\n"
        "Version " PLUGIN_VERSION "\n\n"
        "Compiled on " __DATE__,
        "About " PLUGIN_NAMEVER,
        MB_OK
    );
}

//EXPORT void CALL DllConfig(HWND hParent);

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

    if (!i->status) {
        return;
    }

    Keys->X_AXIS = (int)i->ay * 200 / 250; // 80% range
    Keys->Y_AXIS = (int)i->ax * 200 / 250;

    Keys->A_BUTTON = i->a;
    Keys->B_BUTTON = i->b;
    Keys->Z_TRIG = i->l || i->lt > 128;
    Keys->R_TRIG = i->r || i->rt > 128;
    Keys->L_TRIG = i->z;
    Keys->START_BUTTON = i->start;

    Keys->L_DPAD = i->dleft;
    Keys->R_DPAD = i->dright;
    Keys->U_DPAD = i->dup;
    Keys->D_DPAD = i->ddown;

    Keys->L_CBUTTON = i->cx < -32 || i->y;
    Keys->R_CBUTTON = i->cx >  32 || i->x;
    Keys->D_CBUTTON = i->cy < -32;
    Keys->U_CBUTTON = i->cy >  32;
}

EXPORT void CALL InitiateControllers(HWND hMainWindow, CONTROL Controls[4])
{
    gc_init();
    int err = gc_get_inputs(gamecube);
    if (err)
        return;

    for (int i = 0; i < 4; ++i) {
        Controls[i].Present = !!gamecube[i].status;
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
