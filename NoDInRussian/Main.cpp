#include "Windows.h"
#include <iostream>
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
                    INPUT input = { 0 };
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = (keyboardStruct->vkCode == 's') ? 'z' : 'Z';
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_hMouseHook.get(), nCode, wParam, lParam);
}

DWORD WINAPI MyMouseLogger(LPVOID lpParm)
{
    g_hMouseHook = wil::unique_hhook(SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0));
    if (!g_hMouseHook) {
        auto err = GetLastError();
        std::cout << "Failed setting Windows hook. Error: %u" << err << std::endl;
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
    std::cout << "Main Start" << std::endl;
    DWORD dwThread;

    wil::unique_handle hThread(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyMouseLogger, (LPVOID)argv[0], NULL, &dwThread));
    if (hThread) {
        std::cout << "Waiting for thread"<< std::endl;
        const auto waitRes = WaitForSingleObject(hThread.get(), INFINITE);
        if (waitRes == WAIT_OBJECT_0) {
            // signalned
            std::cout << "signaled" << std::endl;
        } else if (waitRes == WAIT_FAILED) {
            std::cout << "wait failed: " << GetLastError() << std::endl;
        } else {
            std::cout << "other: waitRes " << waitRes << ". Error: " << GetLastError() << std::endl;
        }
    } else {
        std::cout << "Failed to create thread: " << GetLastError() << std::endl;
    }
    return 1;
}