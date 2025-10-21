#pragma once
#include <Windows.h>
#include <string>
#include <filesystem>

class FileManager
{
public:

    static std::string MapPath;
    /**
     * @brief Get the path of the currently loaded module (DLL or executable).
     *
     * @return std::string The module path.
     */
    static std::string GetModulePath();

    /**
     * @brief Get the path to the "mmaps" directory located in the module path.
     *
     * This function assumes that the "mmaps" directory is located in the same directory as the module.
     *
     * @return std::string The "mmaps" directory path.
     */
    static std::string GetMapPath();
};


