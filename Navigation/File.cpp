#include "File.h"
#include <string>
#include <filesystem>

std::string FileManager::GetCurrentWorkingDirectory() {
    try {
        // Get current working directory
        std::filesystem::path cwd = std::filesystem::current_path();

        // Convert to string and ensure trailing backslash
        std::string path = cwd.string();
        if (!path.empty() && path.back() != '\\')
        {
            path += '\\';
        }

        return path;
    }
    catch (const std::exception&) {
        return "";
    }
}

std::string FileManager::GetMapPath() {
    std::string basePath = GetCurrentWorkingDirectory();
    if (basePath.empty())
        return "";

    std::filesystem::path mapPath = std::filesystem::path(basePath) / "mmaps";

    // Check if directory exists
    if (!std::filesystem::exists(mapPath) ||
        !std::filesystem::is_directory(mapPath))
    {
        return "";
    }

    return mapPath.string() + "\\";
}

std::string FileManager::MapPath = "";