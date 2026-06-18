/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

// gui_main.cpp — TimerResolution-style dialog with tray icon and autostart.
//
// Behavior:
//   * Window mirrors Lucas Hale's TimerResolution.exe: a range group, a
//     current group, and Maximum / 1 ms / Default buttons, plus an autostart
//     checkbox.
//   * Closing the window (X) hides it to the tray instead of exiting.
//   * Left-clicking the tray icon restores the window; right-click shows a
//     Show / Exit menu.
//   * A 1-second timer refreshes "Current" and re-applies the requested
//     resolution (per-process drift safeguard on Windows 10 2004+/11).
//   * Every button click persists the chosen value (HKCU\Software\timer-res);
//     "Default" (release) clears it.
//   * Launch with "--min" to start hidden (used by the autostart entry), and
//     "--resume" to re-apply the last persisted value at startup.

#include <windows.h>
#include <shellapi.h>

#include <array>
#include <cstdint>
#include <string_view>

#include "autostart.h"
#include "resource.h"
#include "timer_backend.h"

namespace {

constexpr UINT     kTrayMsg = WM_APP + 1;
constexpr UINT     kTrayId  = 1;
constexpr UINT_PTR kTimerId = 1;

// All mutable UI state for the single dialog instance.
struct AppState {
    HINSTANCE     inst {};
    HWND          dlg {};
    HICON         ownIcon {};      // icon we created via LoadImageW (must DestroyIcon)
    bool          trayShown {};
    std::uint32_t lastDesired {};  // 100-ns units the user requested (0 = released)
};

AppState g_app;

void RefreshLabels(bool full) {
    const auto r = timerres::Query();
    if (!r) return;
    std::array<wchar_t, 32> buf;
    if (full) {
        timerres::FormatMs(r->minimum, buf);
        SetDlgItemTextW(g_app.dlg, IDS_MIN, buf.data());
        timerres::FormatMs(r->maximum, buf);
        SetDlgItemTextW(g_app.dlg, IDS_MAX, buf.data());
    }
    timerres::FormatMs(r->current, buf);
    SetDlgItemTextW(g_app.dlg, IDS_CURRENT, buf.data());
}

void AddTrayIcon() {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_app.dlg;
    nid.uID = kTrayId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = kTrayMsg;
    // LoadImageW-created icons are owned by us and must be DestroyIcon'd; the
    // LoadIconW fallback is a shared system icon that must NOT be destroyed.
    g_app.ownIcon = static_cast<HICON>(
        LoadImageW(g_app.inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                   GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON), 0));
    nid.hIcon = g_app.ownIcon ? g_app.ownIcon : LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Timer Resolution");
    Shell_NotifyIconW(NIM_ADD, &nid);
    g_app.trayShown = true;
}

void RemoveTrayIcon() {
    if (g_app.trayShown) {
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = g_app.dlg;
        nid.uID = kTrayId;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        g_app.trayShown = false;
    }
    if (g_app.ownIcon) {
        DestroyIcon(g_app.ownIcon);
        g_app.ownIcon = nullptr;
    }
}

void ShowMainWindow() {
    // SW_RESTORE (not SW_SHOW) so a minimized window un-minimizes too.
    ShowWindow(g_app.dlg, SW_RESTORE);
    SetForegroundWindow(g_app.dlg);
}

// Apply a fixed desired value (kRelease for "Default") and persist it so a
// later --resume can restore it (kRelease clears the stored value).
void ApplyDesired(std::uint32_t desired) {
    g_app.lastDesired = desired;
    timerres::Set(desired);
    autostart::StoreLastValue(desired);
    RefreshLabels(false);
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_SHOW, L"Show");
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit");
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_app.dlg);  // so the menu dismisses on outside click
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_app.dlg, nullptr);
    DestroyMenu(menu);
}

INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_INITDIALOG:
            g_app.dlg = dlg;
            RefreshLabels(true);
            if (autostart::IsEnabled())
                CheckDlgButton(dlg, IDC_AUTORUN, BST_CHECKED);
            SetTimer(dlg, kTimerId, 1000, nullptr);
            AddTrayIcon();
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDB_MAXIMUM:
                    if (const auto applied = timerres::SetMaximum()) {
                        g_app.lastDesired = *applied;
                        autostart::StoreLastValue(*applied);
                    }
                    RefreshLabels(false);
                    return TRUE;
                case IDB_DEFHI:
                    ApplyDesired(timerres::kOneMs);
                    return TRUE;
                case IDB_4MS:
                    ApplyDesired(timerres::kFourMs);
                    return TRUE;
                case IDB_DEFAULT:
                    ApplyDesired(timerres::kRelease);
                    return TRUE;
                case IDC_AUTORUN:
                    if (IsDlgButtonChecked(dlg, IDC_AUTORUN) == BST_CHECKED)
                        autostart::Enable();
                    else
                        autostart::Disable();
                    return TRUE;
                case IDM_SHOW:
                    ShowMainWindow();
                    return TRUE;
                case IDM_EXIT:
                    DestroyWindow(dlg);
                    return TRUE;
                case IDCANCEL:  // Esc or system close
                    ShowWindow(dlg, SW_HIDE);
                    return TRUE;
            }
            return FALSE;

        case WM_TIMER:
            if (wp == kTimerId) {
                if (g_app.lastDesired != 0)
                    timerres::Set(g_app.lastDesired);
                RefreshLabels(false);
            }
            return TRUE;

        case WM_CLOSE:
            ShowWindow(dlg, SW_HIDE);
            return TRUE;

        case WM_DESTROY:
            KillTimer(dlg, kTimerId);
            RemoveTrayIcon();
            timerres::Set(timerres::kRelease);
            PostQuitMessage(0);
            return TRUE;

        default:
            if (msg == kTrayMsg) {
                if (LOWORD(lp) == WM_LBUTTONUP)
                    ShowMainWindow();
                else if (LOWORD(lp) == WM_RBUTTONUP)
                    ShowTrayMenu();
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// True if any whole command-line token equals `flag` (case-insensitive).
// Tokenized, so "--min" won't match "--minimize" or a path containing it.
bool HasFlag(std::wstring_view flag) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;
    bool found = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], flag.data()) == 0) {
            found = true;
            break;
        }
    }
    LocalFree(argv);
    return found;
}

bool WantsMinimized() { return HasFlag(L"--min"); }
bool WantsResume()    { return HasFlag(L"--resume"); }

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int) {
    g_app.inst = inst;
    if (!timerres::Init()) {
        MessageBoxW(nullptr,
                    L"ntdll NtQuery/NtSetTimerResolution are unavailable.\n"
                    L"This Windows build is not supported.",
                    L"Timer Resolution", MB_ICONERROR | MB_OK);
        return 2;
    }

    // Restore the last persisted value before the UI comes up, so the dialog's
    // first refresh shows it and the 1-second timer keeps re-applying it.
    if (WantsResume()) {
        if (const auto v = autostart::LoadLastValue()) {
            timerres::Set(v);
            g_app.lastDesired = v;
        }
    }

    // Modeless dialog so we own the message loop (needed for the tray icon).
    HWND dlg = CreateDialogParamW(inst, MAKEINTRESOURCEW(IDD_MAIN), nullptr,
                                  DlgProc, 0);
    if (!dlg) return 1;

    ShowWindow(dlg, WantsMinimized() ? SW_HIDE : SW_SHOW);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dlg, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    return 0;
}
