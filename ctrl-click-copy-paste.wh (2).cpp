// ==WindhawkMod==
// @id              ctrl-click-copy-paste
// @name            Ctrl + Click Copy / Double Click Paste
// @description     Ctrl + single click = copy, Ctrl + double click = paste
// @version         3.1
// @author          chmuzammil904-a11y
// @github          https://github.com/chmuzammil904-a11y
// @homepage        https://github.com/chmuzammil904-a11y/ctrl-click-copy-paste
// @license         MIT
// @include         explorer.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Ctrl + Click Copy / Double Click Paste

Hold **Ctrl** and click the **right mouse button** once to copy the selected
text (same as Ctrl+C). Hold **Ctrl** and click twice quickly to paste
(same as Ctrl+V).

The trigger button (right/middle), modifier key (Ctrl/Shift), double click
timing and an optional beep can be changed in the mod settings.

Note: single click copy is delayed by the double click window (350 ms by
default) because the mod has to wait to see whether a second click follows.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- triggerButton: right
  $name: Trigger mouse button
  $options:
  - right: Right button
  - middle: Middle button
- modifier: ctrl
  $name: Modifier key
  $options:
  - ctrl: Ctrl
  - shift: Shift
- doubleClickMs: 350
  $name: Double click window (milliseconds)
  $description: If a second click arrives within this time, paste is performed, otherwise copy
- beep: false
  $name: Beep on copy / paste
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <wchar.h>

static HHOOK g_hook = nullptr;
static HANDLE g_thread = nullptr;
static HANDLE g_readyEvent = nullptr;
static DWORD g_threadId = 0;
static bool g_swallowUp = false;

static bool g_pendingCopy = false;
static UINT_PTR g_timerId = 0;

// Settings
static volatile LONG g_useMiddle = 0;
static volatile LONG g_useShift = 0;
static volatile LONG g_beep = 0;
static volatile LONG g_delayMs = 350;

static void LoadSettings() {
    PCWSTR btn = Wh_GetStringSetting(L"triggerButton");
    g_useMiddle = (btn && wcscmp(btn, L"middle") == 0) ? 1 : 0;
    Wh_FreeStringSetting(btn);

    PCWSTR mod = Wh_GetStringSetting(L"modifier");
    g_useShift = (mod && wcscmp(mod, L"shift") == 0) ? 1 : 0;
    Wh_FreeStringSetting(mod);

    g_beep = Wh_GetIntSetting(L"beep") ? 1 : 0;

    int ms = Wh_GetIntSetting(L"doubleClickMs");
    if (ms < 100) ms = 100;
    if (ms > 1000) ms = 1000;
    g_delayMs = ms;
}

static bool IsDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static bool ModifierMatches() {
    bool ctrl = IsDown(VK_CONTROL);
    bool shift = IsDown(VK_SHIFT);
    bool alt = IsDown(VK_MENU);

    if (alt) return false;
    if (g_useShift) return shift && !ctrl;
    return ctrl && !shift;
}

// Sends Ctrl+<key> (C = copy, V = paste)
static void SendCtrlCombo(WORD key) {
    INPUT in[8] = {};
    int n = 0;

    bool shiftHeld = IsDown(VK_SHIFT);
    bool ctrlHeld = IsDown(VK_CONTROL);

    if (shiftHeld) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_SHIFT;
        in[n].ki.dwFlags = KEYEVENTF_KEYUP;
        n++;
    }
    if (!ctrlHeld) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_CONTROL;
        n++;
    }

    in[n].type = INPUT_KEYBOARD;
    in[n].ki.wVk = key;
    n++;

    in[n].type = INPUT_KEYBOARD;
    in[n].ki.wVk = key;
    in[n].ki.dwFlags = KEYEVENTF_KEYUP;
    n++;

    if (!ctrlHeld) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_CONTROL;
        in[n].ki.dwFlags = KEYEVENTF_KEYUP;
        n++;
    }
    if (shiftHeld) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_SHIFT;
        n++;
    }

    SendInput(n, in, sizeof(INPUT));
}

static void DoCopy() {
    SendCtrlCombo('C');
    if (g_beep) MessageBeep(MB_OK);
}

static void DoPaste() {
    SendCtrlCombo('V');
    if (g_beep) MessageBeep(MB_ICONASTERISK);
}

static void HandleClick() {
    if (g_pendingCopy) {
        // Second click arrived: cancel copy, paste instead
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
        g_pendingCopy = false;
        DoPaste();
    } else {
        // First click: wait, copy if no second click follows
        g_pendingCopy = true;
        g_timerId = SetTimer(nullptr, 0, (UINT)g_delayMs, nullptr);
        if (!g_timerId) {
            g_pendingCopy = false;
            DoCopy();
        }
    }
}

static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        WPARAM downMsg = g_useMiddle ? WM_MBUTTONDOWN : WM_RBUTTONDOWN;
        WPARAM upMsg = g_useMiddle ? WM_MBUTTONUP : WM_RBUTTONUP;

        if (wParam == downMsg && ModifierMatches()) {
            g_swallowUp = true;
            PostThreadMessage(g_threadId, WM_APP, 0, 0);
            return 1;
        }
        if (wParam == upMsg && g_swallowUp) {
            g_swallowUp = false;
            return 1;
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

static DWORD WINAPI HookThread(LPVOID) {
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    g_threadId = GetCurrentThreadId();

    g_hook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc,
                              GetModuleHandle(nullptr), 0);
    SetEvent(g_readyEvent);

    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_APP) {
            HandleClick();
        } else if (msg.message == WM_TIMER && g_pendingCopy &&
                   msg.wParam == g_timerId) {
            KillTimer(nullptr, g_timerId);
            g_timerId = 0;
            g_pendingCopy = false;
            DoCopy();
        }
    }

    if (g_timerId) {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
    if (g_hook) {
        UnhookWindowsHookEx(g_hook);
        g_hook = nullptr;
    }
    return 0;
}

BOOL Wh_ModInit() {
    LoadSettings();

    g_readyEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    g_thread = CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
    if (!g_thread) {
        return FALSE;
    }
    WaitForSingleObject(g_readyEvent, 5000);
    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}

void Wh_ModUninit() {
    if (g_thread) {
        PostThreadMessage(g_threadId, WM_QUIT, 0, 0);
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = nullptr;
    }
    if (g_readyEvent) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
    }
}
