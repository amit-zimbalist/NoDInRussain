#include "Windows.h"
#include <iostream>
#include <thread>
#include "wil/win32_helpers.h"

wil::unique_hhook g_hMouseHook;

bool IsCurrentKeyboardLayoutHebrew()
{
    // Get the handle of the foreground window
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        std::cerr << "Failed to get the foreground window." << std::endl;
        return false;
    }

    // Get the thread ID of the foreground window
    DWORD threadId = GetWindowThreadProcessId(hwnd, nullptr);

    // Get the keyboard layout for the thread of the foreground window
    HKL hkl = GetKeyboardLayout(threadId);

    // Extract the language identifier (LANGID) from HKL
    LANGID langId = LOWORD(hkl);

    // Check if the language is Hebrew
    return PRIMARYLANGID(langId) == LANG_HEBREW;
}


LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        const auto keyboardStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (keyboardStruct != NULL) {
            if (wParam == WM_KEYDOWN)
            {
                if ((keyboardStruct->vkCode == 's' || keyboardStruct->vkCode == 'S') && IsCurrentKeyboardLayoutHebrew()) {
                    INPUT input{};
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = 'Z';
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_hMouseHook.get(), nCode, wParam, lParam);
}

DWORD WINAPI MyKeboardLogger(LPVOID)
{
    g_hMouseHook = wil::unique_hhook(SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0));
    if (!g_hMouseHook) {
        std::cout << "Failed setting Windows hook. Error: %u" << GetLastError() << std::endl;
    }

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    g_hMouseHook.reset();
    return 0;
}

int main(int argc, char *argv[]) {
    std::thread thread(MyKeboardLogger, argv[0]);
    thread.join();
    return 1;
}