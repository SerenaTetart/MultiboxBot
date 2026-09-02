#include "Marker.h"
#include <iostream>
#include "Pathfinder.h"

std::map<Marker, bool> MarkerCreator::Marks;
std::vector<std::string> MarkerCreator::RemoveMarkerByName;


void MarkerCreator::AddMarker(unsigned int mapid, const char* name, Vector3 position, float radius, Area type)
{
    if (true)
    {
        std::cout << "[Marker] Adding " << name << " MapID " << mapid << " x: " << position.X << " y: " << position.Y << " z: " << position.Z << " radius: "
            << radius << " Type: " << type << std::endl;
    }
    Marker marker = { mapid, name, position, radius, type };
    Marks[marker] = false;
}

void MarkerCreator::RemoveMarker(std::string name)
{
    RemoveMarkerByName.push_back(name);
}

void MarkerCreator::Remove(
    dtNavMeshQuery* meshQuery,
    const dtNavMesh* navmesh,
    dtQueryFilter query)
{
    if (RemoveMarkerByName.empty()) return;
    std::vector<std::string> processedNames;

    for (auto it = Marks.begin(); it != Marks.end(); ) {
        Marker marker = it->first;

        auto removeRequest = std::find(RemoveMarkerByName.begin(), RemoveMarkerByName.end(), marker.Name);

        if (removeRequest == RemoveMarkerByName.end()) {
            ++it;
            continue;
        }

        if (!it->second) {
            processedNames.push_back(marker.Name);
            it = Marks.erase(it);
            continue;
        }

        Marker restoreMarker = marker;
        restoreMarker.Type = Area::Walkable;

        if (PathFinder::ApplyCircleBlacklistToPolys(meshQuery, navmesh, query, restoreMarker)) {
            std::cout << "Removing " << marker.Name << std::endl;

            processedNames.push_back(marker.Name);
            it = Marks.erase(it);
        }
        else ++it;
    }

    for (const auto& name : processedNames) {
        RemoveMarkerByName.erase(
            std::remove(
                RemoveMarkerByName.begin(),
                RemoveMarkerByName.end(),
                name
            ),
            RemoveMarkerByName.end()
        );
    }
}

void MarkerCreator::Apply(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query)
{
    for (auto& pair : Marks)
    {
        if (!pair.second)
        {
            std::cout << "Blacklisting  " << std::string(pair.first.Name) << std::endl;
            PathFinder::ApplyCircleBlacklistToPolys(meshQuery, navmesh, query, pair.first);
            pair.second = true; // Update the value associated with the key to true
        }
    }
}