#include "Navigation.h"
#include <iostream>

HMODULE Navigation::hModule = nullptr;
Navigation::CalculatePathFunc Navigation::calculatePath = nullptr;
Navigation::FreePathArrFunc Navigation::freePathArr = nullptr;
Navigation::AddBlackListFunc Navigation::addBlackList = nullptr;
Navigation::RemoveFromBlacklistFunc Navigation::removeFromBlacklist = nullptr;
Navigation::NavigationDebuggerFunc Navigation::navigationDebugger = nullptr;

bool Navigation::Load(const std::string& dllPath) {
    hModule = LoadLibraryA(dllPath.c_str());
    if (!hModule) {
        DWORD error = GetLastError();
        std::cout << "Failed to load DLL: " << dllPath << std::endl;
        std::cout << "Error code: " << error << std::endl;
        LPVOID errorMsg;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0,
            (LPWSTR)&errorMsg,
            0,
            NULL
        );
        std::cout << "Error message: " << (char*)errorMsg << std::endl;
        LocalFree(errorMsg);
        return false;
    }

    calculatePath = (CalculatePathFunc)GetProcAddress(hModule, "CalculatePath");
    if (!calculatePath) {
        std::cout << "Failed to get CalculatePath function." << std::endl;
        return false;
    }

    freePathArr = (FreePathArrFunc)GetProcAddress(hModule, "FreePathArr");
    if (!freePathArr) {
        std::cout << "Failed to get FreePathArr function." << std::endl;
        return false;
    }

    addBlackList = (AddBlackListFunc)GetProcAddress(hModule, "Blacklist");
    if (!addBlackList) {
        std::cout << "Failed to get Blacklist function." << std::endl;
        return false;
    }

    navigationDebugger = (NavigationDebuggerFunc)GetProcAddress(hModule, "Debugger");
    if (!navigationDebugger) {
        std::cout << "Failed to get Debugger function." << std::endl;
        return false;
    }

    removeFromBlacklist = (RemoveFromBlacklistFunc)GetProcAddress(hModule, "___Remove");
    if (!removeFromBlacklist) {
        std::cout << "Failed to get ___Remove function." << std::endl;
        return false;
    }

    return true;
}

Vector3 Navigation::ComputePath(unsigned int mapId, const Vector3& start, const Vector3& end) {
    int length = 0;
    Vector3* path = calculatePath(mapId, start, end, &length);

    if (!path || length <= 0) {
        return start;
    }

    //std::cout << "path[0]: " << path[0].X << "," << path[0].Y << "," << path[0].Z << "\n";

    Vector3 nextpos = path[0];
    for (int i = 1; i < length && nextpos.DistanceTo(start) < 2.0f; ++i) {
        nextpos = path[i];
    }

    return nextpos;
}

void Navigation::FreePathArr(Vector3* pathArr) {
    freePathArr(pathArr);
}

void Navigation::AddBlackList(unsigned int mapId, const std::string& name, const Vector3& p, float radius, unsigned int type) {
    addBlackList(mapId, name.c_str(), p, radius, type);
}

void Navigation::RemoveFromBlacklist(const std::string& name) {
    removeFromBlacklist(name.c_str());
}

void Navigation::NavigationDebugger(bool onOrOff) {
    navigationDebugger(onOrOff);
}
