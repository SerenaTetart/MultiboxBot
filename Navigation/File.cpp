#include "File.h"
#include <iostream>
using namespace std;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
std::string FileManager::GetModulePath()
{
    WCHAR DllPath[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));
    wstring ws(DllPath);
    string pathAndFile(ws.begin(), ws.end());
    char* c = const_cast<char*>(pathAndFile.c_str());
    int strLength = strlen(c);
    int lastOccur = 0;
    for (int i = 0; i < strLength; i++)
    {
        if (c[i] == '\\') lastOccur = i;
    }
    string ModulePath = pathAndFile.substr(0, lastOccur + 1);

    return ModulePath;
}

std::string FileManager::GetMapPath()
{
    std::string ModulePath = GetModulePath();
   
    ModulePath += "mmaps\\";

    // Check if the directory exists
    if (!std::filesystem::exists(ModulePath)) {
        return ""; // Return empty string if the directory doesn't exist
    }

    return ModulePath;
}

std::string FileManager::MapPath = "";
