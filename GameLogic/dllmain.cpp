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