#include "Pathfinder.h"
#include "AreaEnums.h"
#include "Defines.h"
#include <array>
#include <cmath>
#include <DetourCommon.h>

PathFinder::PathFinder(unsigned int mapId, unsigned int instanceId) :
    polyLength(0), pointPathLimit(MAX_POINT_PATH_LENGTH),
    mapId(mapId), InstanceId(instanceId), navMesh(NULL), navMeshQuery(NULL), pathPoints(0)
{
    Mapper* map = MapperHandle::MapHandle();
    navMesh = map->GetNavMesh(mapId);
    navMeshQuery = map->GetNavMeshQuery(mapId, InstanceId);
    SetFilters();
    if (navMesh && navMeshQuery)
    {
        MarkerCreator::Remove(navMeshQuery, navMesh, Queryfilter);
        MarkerCreator::Apply(navMeshQuery, navMesh, Queryfilter);
    }
}

bool PathFinder::calculate(float originX, float originY, float originZ,
                           float destX, float destY, float destZ)
{
    clear();
    startPosition = Vector3(originX, originY, originZ);
    endPosition = Vector3(destX, destY, destZ);
    if (!std::isfinite(originX) || !std::isfinite(originY) || !std::isfinite(originZ) ||
        !std::isfinite(destX) || !std::isfinite(destY) || !std::isfinite(destZ))
        return false;
    if (!navMesh || !navMeshQuery || !HaveTile(startPosition) || !HaveTile(endPosition))
        return false;
    return FindPolyPath(startPosition, endPosition);
}

bool PathFinder::FindPolyPath(Vector3 start, Vector3 end)
{
    clear();
    if (!navMeshQuery)
        return false;

    // Keep the coordinate arrays alive for the entire Detour query.
    const auto startCoords = start.ToRecast().ToFloatArray();
    const auto endCoords = end.ToRecast().ToFloatArray();
    const float* startPoint = startCoords.data();
    const float* endPoint = endCoords.data();

    dtPolyRef startPoly = 0;
    dtPolyRef endPoly = 0;
    float startPolyPoint[3]{};
    float endPolyPoint[3]{};
    const dtStatus startStatus = navMeshQuery->findNearestPoly(
        startPoint, searchBoxSize, &Queryfilter, &startPoly, startPolyPoint);
    if (dtStatusFailed(startStatus) || !startPoly)
        return false;

    const dtStatus endStatus = navMeshQuery->findNearestPoly(
        endPoint, searchBoxSize, &Queryfilter, &endPoly, endPolyPoint);
    if (dtStatusFailed(endStatus) || !endPoly)
        return false;

    int pathPolyCount = 0;
    const dtStatus pathStatus = navMeshQuery->findPath(
        startPoly, endPoly, startPolyPoint, endPolyPoint, &Queryfilter,
        pathPolyRefs, &pathPolyCount, MAX_PATH_LENGTH);

    if (dtStatusFailed(pathStatus) ||
        dtStatusDetail(pathStatus, DT_PARTIAL_RESULT) ||
        dtStatusDetail(pathStatus, DT_BUFFER_TOO_SMALL) ||
        dtStatusDetail(pathStatus, DT_OUT_OF_NODES) ||
        pathPolyCount <= 0 || pathPolyCount > MAX_PATH_LENGTH ||
        pathPolyRefs[pathPolyCount - 1] != endPoly)
        return false;

    polyLength = static_cast<unsigned int>(pathPolyCount);
    return FindPath(startPoint, endPoint);
}

bool PathFinder::FindPath(const float* startPoint, const float* endPoint)
{
    if (!navMeshQuery || !startPoint || !endPoint || !polyLength)
        return false;

    std::array<float, MAX_POINT_PATH_LENGTH * VERTEX_SIZE> pathPointArray{};
    std::array<unsigned char, MAX_POINT_PATH_LENGTH> pathPointFlags{};
    int pointCount = 0;
    const dtStatus status = navMeshQuery->findStraightPath(
        startPoint, endPoint, pathPolyRefs, static_cast<int>(polyLength),
        pathPointArray.data(), pathPointFlags.data(), nullptr,
        &pointCount, MAX_POINT_PATH_LENGTH);

    if (dtStatusFailed(status) ||
        dtStatusDetail(status, DT_PARTIAL_RESULT) ||
        dtStatusDetail(status, DT_BUFFER_TOO_SMALL) ||
        pointCount <= 0 || pointCount > MAX_POINT_PATH_LENGTH ||
        !(pathPointFlags[pointCount - 1] & DT_STRAIGHTPATH_END))
        return false;

    for (int i = 0; i < pointCount; ++i)
    {
        const float* pos = &pathPointArray[static_cast<size_t>(i) * VERTEX_SIZE];
        pathPoints.push_back(Vector3(pos[0], pos[1], pos[2]).ToWoW());
    }
    return true;
}
