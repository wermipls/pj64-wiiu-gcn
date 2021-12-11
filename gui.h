#include <windows.h>
#include "resource.h"

INT_PTR CALLBACK dlgproc(HWND diag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            EndDialog(diag, 0);
            break;
        case WM_COMMAND:
            switch (wParam)
            {
                case IDC_SAVE:
                    EndDialog(diag, 0);
                    break;
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
