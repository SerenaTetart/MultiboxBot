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

std::string GetDllPath(HMODULE hModule)
{
    // Buffer to store the DLL path
    std::vector<char> pathBuffer(MAX_PATH);

    // Get the path of the DLL
    std::cout << "OK1\n";
    DWORD result = GetModuleFileNameA(hModule, pathBuffer.data(), pathBuffer.size());
    std::cout << "OK2\n";

    if (result == 0)
    {
        // Handle the error
        std::cout << "Failed to get DLL path. Error code: " << GetLastError() << std::endl;
        return "";
    }
    std::cout << "OK3\n";

    if (result == pathBuffer.size())
    {
        // Handle the case where the buffer was too small
        // If the buffer was too small, reallocate and try again
        pathBuffer.resize(result * 2);
        result = GetModuleFileNameA(hModule, pathBuffer.data(), pathBuffer.size());
        if (result == 0)
        {
            std::cout << "Failed to get DLL path. Error code: " << GetLastError() << std::endl;
            return "";
        }
    }

    // Convert the path to std::string and return
    return std::string(pathBuffer.data(), result);
}

std::string GetDllFolder(HMODULE hModule)
{
    std::string fullPath = GetDllPath(hModule);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos)
    {
        return fullPath.substr(0, pos);
    }
    return ""; // Return an empty string if no directory found
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