#include "CodecInst.h"
#include "resource.h"

#pragma comment(lib, "User32.Lib")
#pragma comment(lib, "Shell32.Lib")

#include <png.h>

BOOL CodecInst::QueryAbout() { return TRUE; }

static INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
            case IDOK:
            EndDialog(hwndDlg, 0);
            break;

            case IDC_HOMEPAGE:
                ShellExecute(NULL, NULL, "http://research.m1stereo.tv/Mpng", NULL, NULL, SW_SHOW);
                break;

            case IDC_EMAIL:
                ShellExecute(NULL, NULL, "mailto:verem@m1stereo.tv", NULL, NULL, SW_SHOW);
                break;
        }
    }
    else if (uMsg == WM_INITDIALOG)
    {
        SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_VER1), PNG_LIBPNG_VER_STRING);
        SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_VER2), ZLIB_VERSION);
    };
    return FALSE;
};

DWORD CodecInst::About(HWND hwnd)
{
    DialogBox(hmoduleMPNG, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDialogProc);
    return ICERR_OK;
}

static INT_PTR CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        CheckDlgButton(hwndDlg, IDC_LOG,
            GetPrivateProfileInt("debug", "log", 0, "mpng.ini") ? BST_CHECKED : BST_UNCHECKED);
    }
    else if (uMsg == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
            case IDOK:
                WritePrivateProfileString("debug", "log",
                    (IsDlgButtonChecked(hwndDlg, IDC_LOG) == BST_CHECKED) ? "1" : "0", "mpng.ini");

            case IDCANCEL:
                EndDialog(hwndDlg, 0);
            break;

            default:
                return AboutDialogProc(hwndDlg, uMsg, wParam, lParam);    // handle email and home-page buttons
        };
    };
    return FALSE;
};

BOOL CodecInst::QueryConfigure() { return TRUE; }

DWORD CodecInst::Configure(HWND hwnd)
{
    DialogBox(hmoduleMPNG, MAKEINTRESOURCE(IDD_CONFIGURE), hwnd, ConfigureDialogProc);
    return ICERR_OK;
}
