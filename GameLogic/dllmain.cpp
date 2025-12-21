// _begingthreadex
#include <process.h>
// std::wstring
#include <string>
// CLR errors
#include <iostream>

#include "Game.h"
#include "Client.h"
#include "MemoryManager.h"

HANDLE g_hThread = NULL;
HANDLE client_thread = NULL;

BOOL running = true;

std::string GetDllFolder(HMODULE hModule) {
	std::vector<char> pathBuffer(MAX_PATH);
	DWORD result = GetModuleFileNameA(hModule, pathBuffer.data(), pathBuffer.size());

    std::string fullPath = std::string(pathBuffer.data(), result);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos);
    }
    return "";
}

DWORD WINAPI ThreadMain(void* pParam) {
    AllocConsole();
    FILE* t = freopen("CONOUT$", "w", stdout);

    std::cout << "Dll injected !\n";

	Client::client_running = true;
	client_thread = CreateThread(NULL, 0, Client::MakeLoop, NULL, 0, NULL);
	ThreadSynchronizer::Init();
	Game::MainLoop();

    return 1;
}

BOOL WINAPI DllMain( HMODULE hModule, DWORD  dwReason, LPVOID lpReserved ) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		srcPath = GetDllFolder(hModule);
        g_hThread = CreateThread(NULL, 0, ThreadMain, NULL, 0, NULL);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		if (g_hThread) {
			Client::client_running = false;
			TerminateThread(g_hThread, 0);
			CloseHandle(g_hThread);
		}
	}
    return TRUE;
}