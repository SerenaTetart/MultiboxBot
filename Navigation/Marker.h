#pragma once
#include "Vector3.h"
#include <map>
#include "AreaEnums.h"
#include <vector>
#include <DetourNavMeshQuery.h>


class Marker
{
public:
    unsigned int Mapid;
    std::string Name;
    Vector3 position;
    float radius;
    Area Type;

    Marker(unsigned int mapid, std::string name, Vector3 position, float radius, Area type)
        : Mapid(mapid), Name(name), position(position), radius(radius), Type(type) {}

    bool operator<(const Marker& other) const
    {
        if (Mapid != other.Mapid) return Mapid < other.Mapid;
        if (std::string(Name) != std::string(other.Name)) return std::string(Name) < std::string(other.Name);
        if (position.X != other.position.X) return position.X < other.position.X;
        if (position.Y != other.position.Y) return position.Y < other.position.Y;
        if (position.Z != other.position.Z) return position.Z < other.position.Z;
        if (radius != other.radius) return radius < other.radius;
        return Type < other.Type;
    }
};

class MarkerCreator
{
public:

    static void AddMarker(unsigned int mapid, const char* name, Vector3 position, float radius, Area type);
    static void RemoveMarker(std::string name);
    static void Apply(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query);
    static void Remove(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query);
    static std::map<Marker, bool> Marks;
    static std::vector<std::string> RemoveMarkerByName;


};