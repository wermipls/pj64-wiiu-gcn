#include <windows.h>
#include <CommCtrl.h>
#include "resource.h"

void init_slider(HWND diag, int id, int min, int max, int val)
{
    HWND slider = GetDlgItem(diag, id);

    SendMessage(slider, TBM_SETRANGE, TRUE, 
                (LPARAM)MAKELONG(min, max));

    SendMessage(slider, TBM_SETPOS, TRUE, 
                (LPARAM)val);
}

void slider_updatecfg(HWND slider)
{
    LRESULT pos = SendMessage(slider, TBM_GETPOS, 0, 0);
    
    switch (GetDlgCtrlID(slider))
    {
        case IDC_SLIDER_RANGE:
            cfg.range = pos;
            break;
        case IDC_SLIDER_TRIGTHRES:
            cfg.trig_thres = pos;
            break;
        case IDC_SLIDER_CSTICKTHRES:
            cfg.cstick_thres = pos;
            break;
        case IDC_SLIDER_DZ:
            cfg.dz = pos;
            break;
    }
}

void init_all(HWND diag)
{
    CheckDlgButton(diag, IDC_SWAPZL, cfg.swap_zl);
    CheckDlgButton(diag, IDC_ANALOGTRIG, cfg.analog_trig);
    CheckDlgButton(diag, IDC_ZL_AS_Z, cfg.zl_as_z);
    CheckDlgButton(diag, IDC_ASYNC, cfg.async);

    int radio = cfg.xy_mode == XY_CBUTTONS ? IDC_XY_CB
                                           : IDC_Y_AS_L;
    CheckRadioButton(diag, IDC_XY_CB, IDC_Y_AS_L, radio);

    init_slider(diag, IDC_SLIDER_RANGE, 0, 100, cfg.range);
    init_slider(diag, IDC_SLIDER_TRIGTHRES, 0, 255, cfg.trig_thres);
    init_slider(diag, IDC_SLIDER_CSTICKTHRES, 0, 127, cfg.cstick_thres);
    init_slider(diag, IDC_SLIDER_DZ, 0, 100, cfg.dz);
    // set those to blank for now because
    // converting to percentage is not implemented
    SetWindowText(GetDlgItem(diag, IDC_LABEL_RANGE), "");
    SetWindowText(GetDlgItem(diag, IDC_LABEL_TRIGTHRES), "");
    SetWindowText(GetDlgItem(diag, IDC_LABEL_CSTICKTHRES), "");
    SetWindowText(GetDlgItem(diag, IDC_LABEL_DZ), "");
}

INT_PTR CALLBACK dlgproc(HWND diag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
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
                case IDC_TESTPOLL:
                    MessageBox(diag, "Not implemented yet!", "sowwie UwU", MB_OK);
                    break;
                case IDC_DEFAULTS:
                    config_defaults();
                    init_all(diag);
                    break;
                case IDC_SAVE:
                    config_save();
                    EndDialog(diag, 0);
                    break;
            }
        case WM_HSCROLL:
            if (LOWORD(wParam) == TB_ENDTRACK) {
                slider_updatecfg((HWND)lParam);
            }
        default:
            return FALSE;
    }

    return TRUE;
}

void config_window(HINSTANCE hinst, HWND parent)
{
    DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG1), parent, dlgproc);
}
