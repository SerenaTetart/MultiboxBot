#include "NavigationManager.h"
#include "Mapper.h"
#include "Pathfinder.h"
#include "File.h"
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cmath>

NavigationManager* NavigationManager::s_singletonInstance = nullptr;
bool NavigationManager::DEBUGMOD = false;

NavigationManager* NavigationManager::GetInstance()
{
    if (!s_singletonInstance)
        s_singletonInstance = new NavigationManager();
    return s_singletonInstance;
}

void NavigationManager::Initialize()
{
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);
}

void NavigationManager::Release()
{
    MapperHandle::MapHandle()->~Mapper();
}

void NavigationManager::FreePathArr(Vector3* pathArr)
{
    delete[] pathArr;
}

void NavigationManager::InitializeMaps(Mapper* manager, unsigned int mapId)
{
    if (manager->zoneMap.contains(mapId) || FileManager::MapPath.empty())
        return;

    std::ostringstream name;
    name << std::setw(3) << std::setfill('0') << mapId;
    const std::string prefix = name.str();

    std::error_code ec;
    std::filesystem::directory_iterator it(FileManager::MapPath, ec), end;
    if (ec)
        return;

    int loaded = 0;
    for (; it != end; it.increment(ec))
    {
        if (ec)
            break;
        if (it->path().extension() != ".mmtile")
            continue;
        const std::string filename = it->path().filename().string();
        if (filename.size() != 14 || filename.compare(0, 3, prefix) != 0)
            continue;
        bool valid = true;
        for (int i = 3; i < 7; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(filename[i])))
            {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;
        const int x = (filename[3] - '0') * 10 + filename[4] - '0';
        const int y = (filename[5] - '0') * 10 + filename[6] - '0';
        if (manager->loadMap(mapId, x, y))
            ++loaded;
    }

    if (loaded > 0 && !ec)
        manager->zoneMap.emplace(mapId, true);
}

void NavigationManager::LoadMap(unsigned int mapId)
{
    InitializeMaps(MapperHandle::MapHandle(), mapId);
}

Vector3* NavigationManager::CalculatePath(unsigned int mapId, Vector3 start, Vector3 end, int* length)
{
    if (!length)
        return nullptr;
    *length = 0;
    if (!std::isfinite(start.X) || !std::isfinite(start.Y) || !std::isfinite(start.Z) ||
        !std::isfinite(end.X) || !std::isfinite(end.Y) || !std::isfinite(end.Z))
        return nullptr;

    LoadMap(mapId);
    PathFinder pathFinder(mapId, 1);
    if (!pathFinder.calculate(start.X, start.Y, start.Z, end.X, end.Y, end.Z))
        return nullptr;

    const std::vector<Vector3>& points = pathFinder.getPath();
    if (points.empty())
        return nullptr;

    *length = static_cast<int>(points.size());
    Vector3* result = new Vector3[points.size()];
    for (size_t i = 0; i < points.size(); ++i)
        result[i] = points[i];
    return result;
}
