#define _WIN32_WINNT 0x0A00

#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <process.h>
#include <string>

std::string getCurrentDir() {
    char buff[MAX_PATH];
    GetModuleFileName(NULL, buff, MAX_PATH);
    std::string::size_type position = std::string(buff).find_last_of("\\/");
    return std::string(buff).substr(0, position);
}

void Start_Inject_Client(const char* pathWoW) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcess(TEXT(pathWoW),
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        std::cout << "FACEPALM !!!" << "\n";
    }
    //std::cout << "pi.dwProcessId: " << pi.dwProcessId << "\n";
    //std::cout << "processHandle: " << pi.hProcess << "\n";

    Sleep(500);

    std::string pathTMP = getCurrentDir() + "\\GameLogic.dll";
    LPCSTR loaderPath = pathTMP.c_str();
    //std::cout << "loaderPath: " << loaderPath << "\n";

    LPVOID loaderPathPtr = VirtualAllocEx(
        pi.hProcess,
        0,
        strlen(loaderPath) + 1,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE);
    //std::cout << "loaderPathPtr: " << loaderPathPtr << "\n";

    Sleep(500);

    DWORD loaderPathsize = sizeof(loaderPath);
    if (!WriteProcessMemory(pi.hProcess, loaderPathPtr, loaderPath, strlen(loaderPath) + 1, 0))
        std::cout << "FACEPALM2 !!!" << "\n";

    const uintptr_t CTM_FIX = 0x860A90;
    int8_t bytes[] = { 0, 0, 0, 0 };
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)CTM_FIX, bytes, sizeof(bytes), 0))
        std::cout << "FACEPALM31 !!!" << "\n";

    const uintptr_t LUA_UNLOCK = 0x494A50;
    int8_t bytes2[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xc3 };
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)LUA_UNLOCK, bytes2, sizeof(bytes2), 0))
        std::cout << "FACEPALM32 !!!" << "\n";

    FARPROC loaderDllPointer = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    //std::cout << "loaderDllPointer: " << loaderDllPointer << "\n";

    Sleep(500);

    CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)loaderDllPointer, loaderPathPtr, 0, 0);

    Sleep(500);

    VirtualFreeEx(pi.hProcess, loaderPathPtr, 0, MEM_RELEASE);
}

int main(int argc, char* argv[], char* envp[]) {
    if (argc == 2) Start_Inject_Client(argv[1]);
    else Start_Inject_Client("C:\\Program Files (x86)\\WoW\\WoW1.12.1\\WoW.exe");
}