#include "gui.h"
#include <stdio.h>
#include <CommCtrl.h>
#include "resource.h"
#include "log.h"
#include "config.h"
#include "gc_adapter.h"
#include "mapping.h"

static struct config cfg_old;
static HINSTANCE hinstance;
static HWND mapping_tab;

int current_controller = 0;

void init_mapping(HWND diag, int id)
{
    HWND pri = GetDlgItem(diag, id);
    HWND sec = GetDlgItem(diag, id + 100);

    for (int i = 0; i < BA_MAX; i++) {
        const char *str = mapping_get_label(i);
        SendMessage(pri, CB_ADDSTRING, 0, (LPARAM)str);
        SendMessage(sec, CB_ADDSTRING, 0, (LPARAM)str);
    }
}

enum MappingButtonAxis *baptr_from_idc(int id)
{
    struct ConfigMapping *cfgmap = &cfg.mapping[current_controller];
    struct Mapping *map = 0;
    int is_sec = 0;
    if (id >= IDC_MAPPING_SECONDARY_A) {
        id -= 100;
        is_sec = 1;
    }

    switch (id)
    {
    case IDC_MAPPING_A: map = &cfgmap->a; break;
    case IDC_MAPPING_B: map = &cfgmap->b; break;
    case IDC_MAPPING_Z: map = &cfgmap->z; break;
    case IDC_MAPPING_L: map = &cfgmap->l; break;
    case IDC_MAPPING_R: map = &cfgmap->r; break;
    case IDC_MAPPING_START: map = &cfgmap->start; break;

    case IDC_MAPPING_UP:    map = &cfgmap->analog_up; break;
    case IDC_MAPPING_DOWN:  map = &cfgmap->analog_down; break;
    case IDC_MAPPING_LEFT:  map = &cfgmap->analog_left; break;
    case IDC_MAPPING_RIGHT: map = &cfgmap->analog_right; break;

    case IDC_MAPPING_CUP:    map = &cfgmap->c_up; break;
    case IDC_MAPPING_CDOWN:  map = &cfgmap->c_down; break;
    case IDC_MAPPING_CLEFT:  map = &cfgmap->c_left; break;
    case IDC_MAPPING_CRIGHT: map = &cfgmap->c_right; break;

    case IDC_MAPPING_DUP:    map = &cfgmap->d_up; break;
    case IDC_MAPPING_DDOWN:  map = &cfgmap->d_down; break;
    case IDC_MAPPING_DLEFT:  map = &cfgmap->d_left; break;
    case IDC_MAPPING_DRIGHT: map = &cfgmap->d_right; break;
    }

    if (map) {
        return is_sec ? &map->sec : &map->pri;
    }

    return 0;
}

void update_mapping_selection(HWND diag, int id, struct Mapping map)
{
    HWND pri = GetDlgItem(diag, id);
    HWND sec = GetDlgItem(diag, id + 100);

    SendMessage(pri, CB_SETCURSEL, map.pri, 0);
    SendMessage(sec, CB_SETCURSEL, map.sec, 0);
}

void init_mappings(HWND diag)
{
    for (int i = IDC_MAPPING_A; i <= IDC_MAPPING_RIGHT; i++) {
        init_mapping(diag, i);

        struct Mapping *map = (struct Mapping *)baptr_from_idc(i);
        update_mapping_selection(diag, i, *map);
    }
}

void update_mapping_config(HWND diag, int id) {
    enum MappingButtonAxis *ba = baptr_from_idc(id);
    HWND h = GetDlgItem(diag, id);

    *ba = SendMessage(h, CB_GETCURSEL, 0, 0);
}

INT_PTR CALLBACK mapping_dlgproc(HWND diag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        init_mappings(diag);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            int id = LOWORD(wParam);
            if (id >= IDC_MAPPING_A && id <= IDC_MAPPING_SECONDARY_RIGHT) {
                update_mapping_config(diag, id);
            }
        } 
    default:
        return FALSE;
    }

    return TRUE;
}

void init_tabs(HWND diag)
{
    HWND tab = GetDlgItem(diag, IDC_TABCONTROL);

    for (int i = 0; i < 4; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Controller %d", i+1);

        TCITEM item;
        item.mask = TCIF_TEXT;
        item.pszText = buf;

        TabCtrl_InsertItem(tab, i, &item);
    }
    
    mapping_tab = CreateDialogA(hinstance, MAKEINTRESOURCE(IDD_MAPPING_TAB), diag, mapping_dlgproc);

    RECT rc_client, rc_window;
    GetClientRect(tab, &rc_client);
    TabCtrl_AdjustRect(tab, FALSE, &rc_client);

    GetWindowRect(tab, &rc_window);
    ScreenToClient(diag, (LPPOINT)&rc_window);

    OffsetRect(&rc_client, rc_window.left, rc_window.top);

    MoveWindow(mapping_tab,
        rc_client.left, rc_client.top, rc_client.right-rc_client.left, rc_client.bottom-rc_client.top, FALSE);

    ShowWindow(mapping_tab, SW_SHOW);
}

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
        cfg.stick_a2d_thres = pos;
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
    CheckDlgButton(diag, IDC_ASYNC, cfg.async);

    init_slider(diag, IDC_SLIDER_RANGE, 0, 100, cfg.range);
    init_slider(diag, IDC_SLIDER_TRIGTHRES, 0, 255, cfg.trig_thres);
    init_slider(diag, IDC_SLIDER_CSTICKTHRES, 0, 127, cfg.stick_a2d_thres);
    init_slider(diag, IDC_SLIDER_DZ, 0, 100, cfg.dz);

    print_percent(diag, IDC_LABEL_RANGE, cfg.range);
    print_percent(diag, IDC_LABEL_TRIGTHRES, cfg.trig_thres*100/255);
    print_percent(diag, IDC_LABEL_CSTICKTHRES, cfg.stick_a2d_thres*100/127);
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
        init_tabs(diag);
        break;
    case WM_CLOSE:
        DestroyWindow(mapping_tab);
        EndDialog(diag, 0);
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDC_ASYNC:
            cfg.async = IsDlgButtonChecked(diag, IDC_ASYNC) ? 1 : 0;
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
        case IDC_CANCEL:
            cfg = cfg_old;
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
    hinstance = hinst;
    DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG1), parent, dlgproc);
}
