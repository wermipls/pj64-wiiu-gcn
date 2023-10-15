#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Shlwapi.h>
#include "zilmar_controller_1.0.h"
#include "gc_adapter.h"
#include "config.h"
#include "gui.h"
#include "util.h"
#include "plugin_info.h"
#include "mapping.h"
#include "log.h"
#include "pak.h"

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

    int port = get_port_mapping(Control);
    int err = gc_get_inputs(port, &i);
    if (err)
        return;

    Keys->Value = 0;

    if (!gc_is_present(i.status)) {
        return;
    }

    Control = cfg.single_mapping ? 0 : Control;

    gc_inputs id = i;
    process_inputs_digital(&id);
    process_inputs_analog(&i);

    // mappings here LOL
    Keys->A_BUTTON = get_mapping_state(&i, &id, cfg.controller[Control].a, 0);
    Keys->B_BUTTON = get_mapping_state(&i, &id, cfg.controller[Control].b, 0);
    Keys->Z_TRIG = get_mapping_state(&i, &id, cfg.controller[Control].z, 0);
    Keys->L_TRIG = get_mapping_state(&i, &id, cfg.controller[Control].l, 0);
    Keys->R_TRIG = get_mapping_state(&i, &id, cfg.controller[Control].r, 0);
    Keys->START_BUTTON = get_mapping_state(&i, &id, cfg.controller[Control].start, 0);

    Keys->L_CBUTTON = get_mapping_state(&i, &id, cfg.controller[Control].c_left, 0);
    Keys->R_CBUTTON = get_mapping_state(&i, &id, cfg.controller[Control].c_right, 0);
    Keys->D_CBUTTON = get_mapping_state(&i, &id, cfg.controller[Control].c_down, 0);
    Keys->U_CBUTTON = get_mapping_state(&i, &id, cfg.controller[Control].c_up, 0);

    Keys->L_DPAD = get_mapping_state(&i, &id, cfg.controller[Control].d_left, 0);
    Keys->R_DPAD = get_mapping_state(&i, &id, cfg.controller[Control].d_right, 0);
    Keys->D_DPAD = get_mapping_state(&i, &id, cfg.controller[Control].d_down, 0);
    Keys->U_DPAD = get_mapping_state(&i, &id, cfg.controller[Control].d_up, 0);

    Keys->X_AXIS = get_mapping_state(&i, &id, cfg.controller[Control].analog_up, 1)
                 - get_mapping_state(&i, &id, cfg.controller[Control].analog_down, 1);

    Keys->Y_AXIS = get_mapping_state(&i, &id, cfg.controller[Control].analog_right, 1)
                 - get_mapping_state(&i, &id, cfg.controller[Control].analog_left, 1);
}

EXPORT void CALL InitiateControllers(HWND hMainWindow, CONTROL Controls[4])
{
    dlog(LOG_INFO, "InitiateControllers()");
    gc_init(cfg.async);
    if (gc_get_init_error() == GCERR_LIBUSB_OPEN) {
        MessageBox(hMainWindow, 
            "Failed to open the adapter.\n\n"
            "Make sure the correct driver is installed and the adapter is plugged in.",
            PLUGIN_NAME " error", MB_ICONERROR | MB_OK);
        return;
    }

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
        int port = get_port_mapping(i);
        int status = gc[port].status;
        int mi = cfg.single_mapping ? 0 : port;

        if (gc_is_present(status)) {
            Controls[i].Present = cfg.controller[mi].enabled ? TRUE : FALSE;
            dlog(LOG_INFO, "Controller %d (port %d) present, status 0x%X", i, port, status);
            ++concount;
        } else {
            Controls[i].Present = (cfg.controller[mi].force_plugged 
                                   && cfg.controller[mi].enabled) ? TRUE : FALSE;
            dlog(LOG_INFO, "Controller %d (port %d) not available, status 0x%X", i, port, status);
        }

        Controls[i].RawData = FALSE;
        switch (cfg.controller[mi].accessory)
        {
        case ACCESSORY_NONE:
        case ACCESSORY_CPAK:
            Controls[i].Plugin = PLUGIN_NONE + cfg.controller[mi].accessory;
            break;
        case ACCESSORY_RUMBLE:
            Controls[i].RawData = TRUE;
            Controls[i].Plugin = PLUGIN_RAW;
            break;
        }
    }

    if (concount == 0) {
        MessageBox(
            hMainWindow, 
            "No controllers detected.\n\n"
            "Please plug in a controller and make sure port assignment is correct, "
            "then restart the emulator.",
            PLUGIN_NAME " info", MB_OK | MB_ICONINFORMATION
        );
    }
}

EXPORT void CALL ControllerCommand(int Control, BYTE *Command)
{

}

EXPORT void CALL ReadController(int Control, BYTE *Command)
{
    if (Control == -1)
        return; // why make a call in the 1st place if there's nothing to do?

    // from what i understand:
    // Command[0] is command + data length sent by the console
    // Command[1] is length of our response in bytes
    // so the actual data (command then response) only begins at Command[2]

    unsigned char *len_tx   = &Command[0];
    unsigned char *len_rx   = &Command[1];
    unsigned char *cmd      = &Command[2];
    unsigned char *data     = &Command[3];

    switch (*cmd)
    {
    case 0xFF: // controller info/reset
    case 0x00: // controller info
        data[0] = 0x05; // normal n64 controller
        data[1] = 0x00; 
        data[2] = 0x01; // rumble/cpak
        break;
    case 0x01: // controller status
        GetKeys(Control, (BUTTONS*)data); // HACK: this would optimally be a separate func
        break;
    case 0x02: // peripheral read
        pak_read(
            Control,
            data[1] | (data[0] << 8),
            data+2);
        break;
    case 0x03: // peripheral write
        pak_write(
            Control,
            data[1] | ((uint16_t)data[0] << 8),
            data+2);
        break;
    default:
        *len_rx = *len_rx | 0x80;
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
