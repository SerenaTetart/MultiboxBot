#define WIN32_LEAN_AND_MEAN
#include "MemoryManager.h"

#include <windows.h>
#include <iostream>
#include <memory>
#include <utility>

static HWND windowHandle;
static WNDPROC newCallback;
static WNDPROC oldCallback;
static constexpr UINT RUN_ACTION_MESSAGE = WM_USER;

void ThreadSynchronizer::pressKey(unsigned int key) {
    SendMessageW(windowHandle, WM_KEYDOWN, key, 0);
}

void ThreadSynchronizer::releaseKey(unsigned int key) {
    SendMessageW(windowHandle, WM_KEYUP, key, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wparam, LPARAM lparam) {
    if (Msg == RUN_ACTION_MESSAGE && lparam != 0) {
        std::unique_ptr<std::function<void()>> action(
            reinterpret_cast<std::function<void()>*>(lparam)
        );
        std::invoke(*action);
        return 0;
    }

    if (oldCallback) {
        return CallWindowProcW(oldCallback, hwnd, Msg, wparam, lparam);
    }

    return DefWindowProcW(hwnd, Msg, wparam, lparam);
}

void ThreadSynchronizer::Init() {
    windowHandle = FindWindowW(NULL, L"World of Warcraft");
    while (windowHandle == 0) {
        Sleep(250);
        windowHandle = FindWindowW(NULL, L"World of Warcraft");
    }
    std::cout << "windowHandle:" << windowHandle << "\n";
    newCallback = (WNDPROC)&WndProc;
    oldCallback = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(windowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newCallback))
    );
}

void ThreadSynchronizer::RunOnMainThread(std::function<void()> action) {
    auto* actionPtr = new std::function<void()>(std::move(action));
    SendMessageW(windowHandle, RUN_ACTION_MESSAGE, 0, reinterpret_cast<LPARAM>(actionPtr));
}
