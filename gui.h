#include <windows.h>
#include <CommCtrl.h>
#include "resource.h"

static struct config cfg_old;

void init_slider(HWND diag, int id, int min, int max, int val)
{
    HWND slider = GetDlgItem(diag, id);

    SendMessage(slider, TBM_SETRANGE, TRUE, 
                (LPARAM)MAKELONG(min, max));

    SendMessage(slider, TBM_SETPOS, TRUE, 
                (LPARAM)val);
}

void print_percent(HWND diag, int id, int val)
{
    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", val);
    HWND label = GetDlgItem(diag, id);
    SetWindowText(label, buf);
}

void slider_updatecfg(HWND diag, HWND slider)
{
    LRESULT pos = SendMessage(slider, TBM_GETPOS, 0, 0);
    HWND label;

    switch (GetDlgCtrlID(slider))
    {
    case IDC_SLIDER_RANGE:
        cfg.range = pos;
        print_percent(diag, IDC_LABEL_RANGE, pos);
        break;
    case IDC_SLIDER_TRIGTHRES:
        cfg.trig_thres = pos;
        print_percent(diag, IDC_LABEL_TRIGTHRES, pos*100/255);
        break;
    case IDC_SLIDER_CSTICKTHRES:
        cfg.cstick_thres = pos;
        print_percent(diag, IDC_LABEL_CSTICKTHRES, pos*100/127);
        break;
    case IDC_SLIDER_DZ:
        cfg.dz = pos;
        print_percent(diag, IDC_LABEL_DZ, pos);
        break;
    }
}

void init_all(HWND diag)
{
    CheckDlgButton(diag, IDC_SWAPZL, cfg.swap_zl);
    CheckDlgButton(diag, IDC_ANALOGTRIG, cfg.analog_trig);
    CheckDlgButton(diag, IDC_ZL_AS_Z, cfg.zl_as_z);
    CheckDlgButton(diag, IDC_ASYNC, cfg.async);

    int radio;

    switch (cfg.xy_mode)
    {
    case XY_CBUTTONS:    radio = IDC_XY_CB; break;
    case XY_L_4CBUTTONS: radio = IDC_Y_AS_L; break;
    case XY_NONE:        radio = IDC_XY_NONE; break;
    }

    CheckRadioButton(diag, IDC_XY_NONE, IDC_Y_AS_L, radio);

    init_slider(diag, IDC_SLIDER_RANGE, 0, 100, cfg.range);
    init_slider(diag, IDC_SLIDER_TRIGTHRES, 0, 255, cfg.trig_thres);
    init_slider(diag, IDC_SLIDER_CSTICKTHRES, 0, 127, cfg.cstick_thres);
    init_slider(diag, IDC_SLIDER_DZ, 0, 100, cfg.dz);

    print_percent(diag, IDC_LABEL_RANGE, cfg.range);
    print_percent(diag, IDC_LABEL_TRIGTHRES, cfg.trig_thres*100/255);
    print_percent(diag, IDC_LABEL_CSTICKTHRES, cfg.cstick_thres*100/127);
    print_percent(diag, IDC_LABEL_DZ, cfg.dz);
}

void mb_pollrate(HWND parent)
{
    float result = gc_test_pollrate();

    if (result < 0) {
        MessageBox(parent, 
                   "Failed to test the pollrate.\n\n"
                   "Enable async polling, then restart the emulator.",
                   "sowwie UwU", MB_ICONERROR | MB_OK);
    } else {
            char buf[128];
        snprintf(buf, sizeof(buf), 
                 "Measured pollrate: %.1f Hz",
                 result);
        MessageBox(parent, buf, "Mucho texto", MB_ICONINFORMATION | MB_OK);
    }
}

int restart_required()
{
    if (cfg.async != cfg_old.async)
        return 1;

    return 0;
}

INT_PTR CALLBACK dlgproc(HWND diag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        cfg_old = cfg;
        init_all(diag);
        break;
    case WM_CLOSE:
        EndDialog(diag, 0);
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDC_SWAPZL:
            cfg.swap_zl = IsDlgButtonChecked(diag, IDC_SWAPZL) ? 1 : 0;
            break;
        case IDC_ANALOGTRIG:
            cfg.analog_trig = IsDlgButtonChecked(diag, IDC_ANALOGTRIG) ? 1 : 0;
            break;
        case IDC_ZL_AS_Z:
            cfg.zl_as_z = IsDlgButtonChecked(diag, IDC_ZL_AS_Z) ? 1 : 0;
            break;
        case IDC_ASYNC:
            cfg.async = IsDlgButtonChecked(diag, IDC_ASYNC) ? 1 : 0;
            break;
        case IDC_Y_AS_L:
            cfg.xy_mode = XY_L_4CBUTTONS;
            break;
        case IDC_XY_CB:
            cfg.xy_mode = XY_CBUTTONS;
            break;
        case IDC_XY_NONE:
            cfg.xy_mode = XY_NONE;
            break;
        case IDC_TESTPOLL:
            mb_pollrate(diag);
            break;
        case IDC_DEFAULTS:
            config_defaults();
            init_all(diag);
            break;
        case IDC_SAVE:
            config_save();
            if (restart_required()) {
                MessageBox(
                    diag, 
                    "Some changes require emulator restart to take effect.", 
                    "Info", MB_OK | MB_ICONINFORMATION);
            }
            EndDialog(diag, 0);
            break;
        }
        break;
    case WM_HSCROLL:
        slider_updatecfg(diag, (HWND)lParam);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

void config_window(HINSTANCE hinst, HWND parent)
{
    DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG1), parent, dlgproc);
}
