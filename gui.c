#include "gui.h"
#include <stdio.h>
#include <CommCtrl.h>
#include "resource.h"
#include "log.h"
#include "config.h"
#include "gc_adapter.h"
#include "mapping.h"

#define GC_STATUS_REFRESH_MS 200
#define GC_STATUS_REFRESH_ID 1000
#define GC_STATUS_COLOR_OK   RGB(64, 160, 0)
#define GC_STATUS_COLOR_ERR  RGB(192, 0, 64)

static struct config cfg_old;
static HINSTANCE hinstance;
static HWND mapping_tab;

int current_controller = 0;

unsigned int gc_status_color[5] = { 
    0,
    GC_STATUS_COLOR_OK,
    GC_STATUS_COLOR_OK,
    GC_STATUS_COLOR_OK,
    GC_STATUS_COLOR_OK
};

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

void update_mapping_tab(HWND diag)
{
    for (int i = IDC_MAPPING_A; i <= IDC_MAPPING_RIGHT; i++) {
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
                break;
            }
        }
        return FALSE;
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

void refresh_gc_status(HWND diag, int force_refresh)
{
    static const char *gc_init_status[] = {
        "N/A",
        "Not initialized",
        "OK",
        "libusb init fail",
        "Not detected",
        "libusb claim fail"
        "Poll thread error"
    };

    HWND label = GetDlgItem(diag, IDC_GCSTATUS_ADAPTER);

    static enum GCError gcerr_old = -2; 
    enum GCError gcerr = gc_get_init_error();
    const char *status = gc_init_status[gcerr+2];

    if (gcerr_old != gcerr || force_refresh) {
        SetWindowText(label, status);
        gcerr_old = gcerr;
    }

    switch (gcerr)
    {
    case GCERR_NOT_INITIALIZED:
        EnableWindow(label, FALSE);
        break;
    case GCERR_LIBUSB_INIT:
    case GCERR_LIBUSB_OPEN:
    case GCERR_LIBUSB_CLAIM_INTERFACE:
    case GCERR_CREATE_THREAD:
        EnableWindow(label, TRUE);
        gc_status_color[0] = GC_STATUS_COLOR_ERR;
        break;
    case GCERR_OK:
        EnableWindow(label, TRUE);
        gc_status_color[0] = GC_STATUS_COLOR_OK;
    }

    static int status_old[4] = { -1 };
    gc_inputs inputs[4];
    int err = gc_get_all_inputs(inputs);
    int i = 0;
    for (int id = IDC_GCSTATUS_1; id <= IDC_GCSTATUS_4; id++) {
        label = GetDlgItem(diag, id);

        int status;
        if (err) {
            status = 0;
        } else {
            if (gc_is_present(inputs[i].status)) {
                status = 1;
            } else {
                status = 2;
            }
        }

        if (status != status_old[i] || force_refresh) {
            switch (status)
            {
            case 0: // err
                SetWindowText(label, "N/A");
                EnableWindow(label, FALSE);
                break;
            case 1:
                EnableWindow(label, TRUE);
                SetWindowText(label, "OK");
                break;
            case 2:
                EnableWindow(label, FALSE);
                SetWindowText(label, "Not detected");
            }

            status_old[i] = status;
        }

        i++;
    }

}

void init_gc_status(HWND diag)
{
    SetTimer(diag, GC_STATUS_REFRESH_ID, GC_STATUS_REFRESH_MS, NULL);
    refresh_gc_status(diag, 1);
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
        init_gc_status(diag);
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
            update_mapping_tab(mapping_tab);
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
    case WM_TIMER:
        if (wParam == GC_STATUS_REFRESH_ID) {
            refresh_gc_status(diag, 0);
        }
        break;
    case WM_CTLCOLORSTATIC: ;
        HDC hdc = (HDC)wParam;
        int id = GetWindowLong((HWND)lParam, GWL_ID);

        if (id >= IDC_GCSTATUS_ADAPTER && id <= IDC_GCSTATUS_4) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, gc_status_color[id-IDC_GCSTATUS_ADAPTER]);
            return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE); 
        }
        return FALSE;
    case WM_NOTIFY: ;
        LPNMHDR lpnmhdr = (LPNMHDR)lParam; 
        if (lpnmhdr->code == TCN_SELCHANGE) {
            if (lpnmhdr->idFrom == IDC_TABCONTROL) {
                current_controller = TabCtrl_GetCurSel(lpnmhdr->hwndFrom);
                update_mapping_tab(mapping_tab);
                break;
            }
        }
        return FALSE;
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
