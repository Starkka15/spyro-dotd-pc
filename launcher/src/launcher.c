/*
 * The launcher: a settings window for <game>.toml, game file installation,
 * and a Play button. Adapted from the Legend of Spyro: A New Beginning port's
 * launcher for the ReXGlue ports (the game reads its settings as SDK cvars).
 *
 * It sits beside <game>.exe. Nothing here is needed to play: the game runs on
 * its own and finds game\ beside itself.
 *
 *   launcher.exe /import "<folder, drive or image>"   (used by Setup)
 *   launcher.exe /config     writes <game>.toml with defaults if missing
 */
#define COBJMACROS
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "game.h"
#include "config.h"
#include "launcher_res.h"
#include "import.h"

static GameConfig s_cfg;

static const int SIZES[][2] = {
    { 960, 540 },   { 1280, 720 },  { 1280, 800 },  { 1366, 768 },  { 1600, 900 },
    { 1920, 1080 }, { 1920, 1200 }, { 2560, 1440 }, { 3840, 2160 },
};

/* Path of a file beside this .exe. */
static void beside_exe(wchar_t *out, size_t cap, const wchar_t *name)
{
    DWORD n = GetModuleFileNameW(NULL, out, (DWORD)cap);
    wchar_t *slash;
    if (!n || n >= cap) {
        out[0] = 0;
        return;
    }
    slash = wcsrchr(out, L'\\');
    if (slash && (size_t)(slash + 1 - out) + wcslen(name) < cap)
        wcscpy(slash + 1, name);
    else
        out[0] = 0;
}

static void monitor_size(HWND h, int *w, int *hh)
{
    MONITORINFO mi = { sizeof mi };
    *w = 1920;
    *hh = 1080;
    if (GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi)) {
        *w = mi.rcMonitor.right - mi.rcMonitor.left;
        *hh = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
}

static void add_item(HWND dlg, int id, const wchar_t *text, LPARAM data)
{
    LRESULT i = SendDlgItemMessageW(dlg, id, CB_ADDSTRING, 0, (LPARAM)text);
    SendDlgItemMessageW(dlg, id, CB_SETITEMDATA, (WPARAM)i, data);
}

static void select_data(HWND dlg, int id, LPARAM data)
{
    LRESULT n = SendDlgItemMessageW(dlg, id, CB_GETCOUNT, 0, 0), i;
    for (i = 0; i < n; i++)
        if (SendDlgItemMessageW(dlg, id, CB_GETITEMDATA, (WPARAM)i, 0) == data) {
            SendDlgItemMessageW(dlg, id, CB_SETCURSEL, (WPARAM)i, 0);
            return;
        }
    SendDlgItemMessageW(dlg, id, CB_SETCURSEL, 0, 0);
}

static LPARAM selected_data(HWND dlg, int id)
{
    LRESULT i = SendDlgItemMessageW(dlg, id, CB_GETCURSEL, 0, 0);
    return i < 0 ? 0 : (LPARAM)SendDlgItemMessageW(dlg, id, CB_GETITEMDATA, (WPARAM)i, 0);
}

static void fill_sizes(HWND dlg, int cur_w, int cur_h)
{
    int mw, mh, have = 0;
    size_t i;
    wchar_t t[64];

    monitor_size(dlg, &mw, &mh);
    SendDlgItemMessageW(dlg, IDC_WINSIZE, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < sizeof SIZES / sizeof *SIZES; i++) {
        int w = SIZES[i][0], h = SIZES[i][1];
        if ((w > mw || h > mh) && !(w == cur_w && h == cur_h))
            continue;
        swprintf(t, 64, L"%d \x00D7 %d", w, h);
        add_item(dlg, IDC_WINSIZE, t, MAKELPARAM(h, w));
        have |= w == cur_w && h == cur_h;
    }
    if (!have) {           /* a size set by hand in the toml */
        swprintf(t, 64, L"%d \x00D7 %d (current)", cur_w, cur_h);
        add_item(dlg, IDC_WINSIZE, t, MAKELPARAM(cur_h, cur_w));
    }
    select_data(dlg, IDC_WINSIZE, MAKELPARAM(cur_h, cur_w));
}

static void update_enabled(HWND dlg)
{
    BOOL windowed = selected_data(dlg, IDC_MODE) == 0;
    BOOL kb = IsDlgButtonChecked(dlg, IDC_MKB) == BST_CHECKED;
    BOOL mouse = kb && IsDlgButtonChecked(dlg, IDC_MOUSE) == BST_CHECKED;
    EnableWindow(GetDlgItem(dlg, IDC_WINSIZE), windowed);
    EnableWindow(GetDlgItem(dlg, IDC_SIZE_LBL), windowed);
    EnableWindow(GetDlgItem(dlg, IDC_MOUSE), kb);
    EnableWindow(GetDlgItem(dlg, IDC_SENS), mouse);
    EnableWindow(GetDlgItem(dlg, IDC_SENS_LBL), mouse);
    EnableWindow(GetDlgItem(dlg, IDC_SENS_VAL), mouse);
}

/* The sensitivity slider counts in twentieths: 5-80 is 0.25-4.0. */
static void show_sens(HWND dlg)
{
    wchar_t t[16];
    int v = (int)SendDlgItemMessageW(dlg, IDC_SENS, TBM_GETPOS, 0, 0);
    swprintf(t, 16, L"%.2f", v / 20.0);
    SetDlgItemTextW(dlg, IDC_SENS_VAL, t);
}

static void show(HWND dlg, const GameConfig *c)
{
    int s = (int)(c->mnk_sensitivity * 20.0 + 0.5);
    s = s < 5 ? 5 : s > 80 ? 80 : s;
    select_data(dlg, IDC_MODE, c->fullscreen);
    fill_sizes(dlg, c->window_width, c->window_height);
    select_data(dlg, IDC_SCALE, c->resolution_scale);
    select_data(dlg, IDC_BACKEND, c->backend);
    CheckDlgButton(dlg, IDC_VSYNC, c->vsync ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dlg, IDC_MKB, c->mnk_mode ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dlg, IDC_MOUSE, c->mnk_mouse ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dlg, IDC_LOG, c->full_log ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessageW(dlg, IDC_SENS, TBM_SETRANGE, TRUE, MAKELPARAM(5, 80));
    SendDlgItemMessageW(dlg, IDC_SENS, TBM_SETPAGESIZE, 0, 5);
    SendDlgItemMessageW(dlg, IDC_SENS, TBM_SETPOS, TRUE, s);
    show_sens(dlg);
    update_enabled(dlg);
}

static void save(HWND dlg)
{
    LPARAM size = selected_data(dlg, IDC_WINSIZE);
    s_cfg.fullscreen = selected_data(dlg, IDC_MODE) != 0;
    if (size) {
        s_cfg.window_width = HIWORD(size);
        s_cfg.window_height = LOWORD(size);
    }
    s_cfg.resolution_scale = (int)selected_data(dlg, IDC_SCALE);
    s_cfg.backend = (int)selected_data(dlg, IDC_BACKEND);
    s_cfg.vsync = IsDlgButtonChecked(dlg, IDC_VSYNC) == BST_CHECKED;
    s_cfg.mnk_mode = IsDlgButtonChecked(dlg, IDC_MKB) == BST_CHECKED;
    s_cfg.mnk_mouse = IsDlgButtonChecked(dlg, IDC_MOUSE) == BST_CHECKED;
    s_cfg.mnk_sensitivity = SendDlgItemMessageW(dlg, IDC_SENS, TBM_GETPOS, 0, 0) / 20.0;
    s_cfg.full_log = IsDlgButtonChecked(dlg, IDC_LOG) == BST_CHECKED;
    config_save(&s_cfg);
}

/* Saves live in Documents\<game> (the SDK's user folder). */
static void open_saves(HWND dlg)
{
    PWSTR docs = NULL;
    wchar_t dir[MAX_PATH];
    if (FAILED(SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &docs)))
        return;
    swprintf(dir, MAX_PATH, L"%ls\\%hs", docs, GAME_ID);
    CoTaskMemFree(docs);
    if (GetFileAttributesW(dir) != INVALID_FILE_ATTRIBUTES)
        ShellExecuteW(dlg, L"open", dir, NULL, NULL, SW_SHOWNORMAL);
    else
        MessageBoxW(dlg, L"There are no saves yet. The folder is made the first time the game runs.",
                    L"Saves folder", MB_ICONINFORMATION);
}

/* game\ beside the launcher, where Setup puts the game. */
static void game_dir(wchar_t *out)
{
    beside_exe(out, MAX_PATH, L"game");
}

/* Whether the game will find its files: game\ (installed) or game\files
 * (a development tree). */
static int game_present(void)
{
    wchar_t d[MAX_PATH], f[MAX_PATH];
    game_dir(d);
    if (import_installed(d))
        return 1;
    swprintf(f, MAX_PATH, L"%ls\\files", d);
    return import_installed(f);
}

static void show_game_status(HWND dlg)
{
    int ok = game_present();
    SetDlgItemTextW(dlg, IDC_GAME_STATUS, ok ? L"Game files: installed" : L"Game files: not installed yet");
    SetDlgItemTextW(dlg, IDC_GAMEFILES, ok ? L"Reinstall game files..." : L"Install game files...");
}

/* A folder or disc drive (pick_folder) or a disc image file. */
static int pick(HWND dlg, int pick_folder, wchar_t *out)
{
    IFileOpenDialog *fd;
    IShellItem *item;
    PWSTR path = NULL;
    int ok = 0;

    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog,
                                (void **)&fd)))
        return 0;
    if (pick_folder) {
        IFileOpenDialog_SetOptions(fd, FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        IFileOpenDialog_SetTitle(fd, L"The game's folder (the one with default.xex)");
    } else {
        static const COMDLG_FILTERSPEC TYPES[] = {
            { L"Xbox 360 disc images (*.iso, *.xiso)", L"*.iso;*.xiso" }, { L"All files", L"*.*" },
        };
        IFileOpenDialog_SetOptions(fd, FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
        IFileOpenDialog_SetFileTypes(fd, 2, TYPES);
        IFileOpenDialog_SetTitle(fd, L"The game's Xbox 360 disc image");
    }
    if (SUCCEEDED(IFileOpenDialog_Show(fd, dlg)) && SUCCEEDED(IFileOpenDialog_GetResult(fd, &item))) {
        if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path))) {
            wcsncpy(out, path, MAX_PATH - 1);
            out[MAX_PATH - 1] = 0;
            CoTaskMemFree(path);
            ok = 1;
        }
        IShellItem_Release(item);
    }
    IFileOpenDialog_Release(fd);
    return ok;
}

/* Asks where the game is and installs it. */
static void install_game_files(HWND dlg)
{
    static const TASKDIALOG_BUTTON BUTTONS[] = {
        { 100, L"From a disc image\nAn .iso of the Xbox 360 disc (a full image or a trimmed xiso)." },
        { 101, L"From a folder\nThe game's files already extracted on your PC (default.xex at the top)." },
    };
    TASKDIALOGCONFIG tc;
    wchar_t src[MAX_PATH], dest[MAX_PATH], content[512];
    int choice = 0;

    swprintf(content, 512, L"The game's files (%ls) are copied from your own copy of %ls.", GAME_SIZE_TEXT,
             GAME_DISC);
    memset(&tc, 0, sizeof tc);
    tc.cbSize = sizeof tc;
    tc.hwndParent = dlg;
    tc.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
    tc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    tc.pszWindowTitle = L"Install game files";
    tc.pszMainInstruction = L"Where is your copy of the game?";
    tc.pszContent = content;
    tc.pButtons = BUTTONS;
    tc.cButtons = 2;
    if (FAILED(TaskDialogIndirect(&tc, &choice, NULL, NULL)) || (choice != 100 && choice != 101))
        return;
    if (!pick(dlg, choice == 101, src))
        return;
    game_dir(dest);
    import_game(dlg, src, dest, 0);
    show_game_status(dlg);
}

static int play(HWND dlg)
{
    wchar_t exe[MAX_PATH], dir[MAX_PATH], *slash;
    STARTUPINFOW si = { sizeof si };
    PROCESS_INFORMATION pi;

    if (!game_present()) {
        if (MessageBoxW(dlg, L"The game files are not installed yet. Install them now?", GAME_TITLE,
                        MB_YESNO | MB_ICONQUESTION) == IDYES)
            install_game_files(dlg);
        if (!game_present())
            return 0;
    }
    beside_exe(exe, MAX_PATH, L"" GAME_ID ".exe");
    wcscpy(dir, exe);
    slash = wcsrchr(dir, L'\\');
    if (slash)
        *slash = 0;
    if (!exe[0] || !CreateProcessW(exe, NULL, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi)) {
        wchar_t msg[MAX_PATH + 64];
        swprintf(msg, MAX_PATH + 64, L"Could not start the game:\n%ls", exe);
        MessageBoxW(dlg, msg, L"Play", MB_ICONERROR);
        return 0;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
}

static void fill_lists(HWND dlg)
{
    int mw, mh, s;
    wchar_t t[80];

    add_item(dlg, IDC_MODE, L"Windowed", 0);
    add_item(dlg, IDC_MODE, L"Fullscreen (borderless)", 1);
    monitor_size(dlg, &mw, &mh);
    for (s = 1; s <= 3; s++) {
        swprintf(t, 80, L"%dx \x2013 %d\x00D7%d%ls", s, 1280 * s, 720 * s,
                 s == 1 ? L" (original)" : 1280 * s <= mw ? L"" : L" (above this monitor)");
        add_item(dlg, IDC_SCALE, t, s);
    }
    add_item(dlg, IDC_BACKEND, L"Automatic (Direct3D 12)", 0);
    add_item(dlg, IDC_BACKEND, L"Direct3D 12", 1);
    add_item(dlg, IDC_BACKEND, L"Vulkan", 2);
}

static INT_PTR CALLBACK dlg_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG:
        SendMessageW(dlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1)));
        SendMessageW(dlg, WM_SETICON, ICON_SMALL,
                     (LPARAM)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0));
        fill_lists(dlg);
        show(dlg, &s_cfg);
        show_game_status(dlg);
        return TRUE;
    case WM_HSCROLL:
        if ((HWND)lp == GetDlgItem(dlg, IDC_SENS))
            show_sens(dlg);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_MODE:
            if (HIWORD(wp) == CBN_SELCHANGE)
                update_enabled(dlg);
            return TRUE;
        case IDC_MKB:
        case IDC_MOUSE:
            update_enabled(dlg);
            return TRUE;
        case IDC_KEYS:
            /* The bindings are keybind_* lines in the same file. */
            save(dlg);
            ShellExecuteW(dlg, L"open", L"notepad.exe", config_path(), NULL, SW_SHOWNORMAL);
            return TRUE;
        case IDC_SAVES:
            open_saves(dlg);
            return TRUE;
        case IDC_GAMEFILES:
            install_game_files(dlg);
            return TRUE;
        case IDC_DEFAULTS:
            show(dlg, &CONFIG_DEFAULTS);
            return TRUE;
        case IDC_SAVE:
            save(dlg);
            return TRUE;
        case IDOK:
            save(dlg);
            if (play(dlg))
                EndDialog(dlg, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show_cmd)
{
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_BAR_CLASSES | ICC_PROGRESS_CLASS };
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    (void)prev;
    (void)cmd;
    (void)show_cmd;
    InitCommonControlsEx(&icc);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    /* Setup: launcher.exe /import <folder, drive or image>. The exit code is
     * the IMPORT_ result. */
    if (argv && argc >= 3 && !_wcsicmp(argv[1], L"/import")) {
        wchar_t dest[MAX_PATH];
        game_dir(dest);
        return import_game(NULL, argv[2], dest, 1);
    }
    config_load(&s_cfg);    /* writes the default file when there is none */
    /* launcher.exe /config: only that, no window. */
    if (argv && argc >= 2 && !_wcsicmp(argv[1], L"/config"))
        return 0;
    DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_LAUNCHER), NULL, dlg_proc, 0);
    return 0;
}
