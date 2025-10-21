#define WIN32_LEAN_AND_MEAN
#include "MemoryManager.h"

#include <windows.h>
#include <iostream>

static HWND windowHandle;
static WNDPROC newCallback;
static WNDPROC oldCallback;
static std::vector<std::function<void()>> actionQueue;

void SendUserMessage() {
    SendMessageW(windowHandle, WM_USER, 0, 0);
}

void ThreadSynchronizer::pressKey(unsigned int key) {
    SendMessageW(windowHandle, WM_KEYDOWN, key, 0);
}

void ThreadSynchronizer::releaseKey(unsigned int key) {
    SendMessageW(windowHandle, WM_KEYUP, key, 0);
}

int WndProc(HWND hwnd, UINT Msg, WPARAM wparam, LPARAM lparam) {
    if (actionQueue.size() > 0) {
        std::invoke(actionQueue.back());
        actionQueue.pop_back();
    }
    return CallWindowProcW(oldCallback, hwnd, Msg, wparam, lparam);
}

void ThreadSynchronizer::Init() {
    windowHandle = FindWindowW(NULL, L"World of Warcraft");
    while (windowHandle == 0) {
        Sleep(250);
        windowHandle = FindWindowW(NULL, L"World of Warcraft");
    }
    std::cout << "windowHandle:" << windowHandle << "\n";
    newCallback = (WNDPROC)&WndProc;
    oldCallback = (WNDPROC)SetWindowLongW(windowHandle, GWL_WNDPROC, (LONG)newCallback);
}

void ThreadSynchronizer::RunOnMainThread(std::function<void()> action) {
    actionQueue.push_back(action);
    SendUserMessage();
}