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

void MarkerCreator::Remove(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query)
{
    if (RemoveMarkerByName.empty())
        return;

    for (auto it = Marks.begin(); it != Marks.end(); )
    {
        Marker marker = it->first;
        if (it->second && std::find(RemoveMarkerByName.begin(), RemoveMarkerByName.end(), marker.Name) != RemoveMarkerByName.end())
        {

            std::cout << "Removing " << marker.Name << std::endl;
            // Set the type to Walkable before removing the marker
            marker.Type = Area::Walkable;

            // Apply the circle blacklist to polys
            PathFinder::ApplyCircleBlacklistToPolys(meshQuery, navmesh, query, marker);

            // Remove the marker from Marks
            it = Marks.erase(it);
        }
        else
        {
            ++it;
        }
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