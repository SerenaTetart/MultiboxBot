#pragma once

#include <windows.h>
#include "Vector3.h"

class Navigation {
public:
    static bool Load(const std::string& dllPath);
    static Vector3 ComputePath(unsigned int mapId, const Vector3& start, const Vector3& end);
    static void FreePathArr(Vector3* pathArr);
    static void AddBlackList(unsigned int mapId, const std::string& name, const Vector3& p, float radius, unsigned int type);
    static void RemoveFromBlacklist(const std::string& name);
    static void NavigationDebugger(bool onOrOff);

private:
    static HMODULE hModule;
    typedef Vector3* (__cdecl* CalculatePathFunc)(unsigned int, Vector3, Vector3, int*);
    typedef void(__cdecl* FreePathArrFunc)(Vector3*);
    typedef void(__cdecl* AddBlackListFunc)(unsigned int, const char*, Vector3, float, unsigned int);
    typedef void(__cdecl* RemoveFromBlacklistFunc)(const char*);
    typedef void(__cdecl* NavigationDebuggerFunc)(bool);

    static CalculatePathFunc calculatePath;
    static FreePathArrFunc freePathArr;
    static AddBlackListFunc addBlackList;
    static RemoveFromBlacklistFunc removeFromBlacklist;
    static NavigationDebuggerFunc navigationDebugger;
};