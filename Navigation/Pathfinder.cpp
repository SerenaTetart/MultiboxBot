#include "Pathfinder.h"
#include "AreaEnums.h"
#include "Defines.h"
#include <DetourCommon.h>


PathFinder::PathFinder(unsigned int mapId, unsigned int instanceId) :
	polyLength(0), pointPathLimit(MAX_POINT_PATH_LENGTH),
	mapId(mapId), InstanceId(instanceId), navMesh(NULL), navMeshQuery(NULL), pathPoints(0)
{
    //SetFilters();
	Mapper* Map = MapperHandle::MapHandle();
	navMesh = Map->GetNavMesh(mapId);
    navMeshQuery = Map->GetNavMeshQuery(mapId, InstanceId);

    SetFilters();

    MarkerCreator::Remove(navMeshQuery, navMesh, Queryfilter);
    MarkerCreator::Apply(navMeshQuery, navMesh, Queryfilter);
  
}


bool PathFinder::calculate(float originX, float originY, float originZ, float destX, float destY, float destZ)
{
	startPosition = Vector3(originX, originY, originZ);
	endPosition = Vector3(destX, destY, destZ);


	if (!navMesh || !navMeshQuery || !HaveTile(startPosition) || !HaveTile(endPosition))
	{
        std::cout << "error > PathFinder::calculate" << std::endl;
		clear();
		return false;
	}
    FindPolyPath(startPosition, endPosition);
}


bool PathFinder::FindPolyPath(Vector3 Start, Vector3 End)
{
    float* startPoint = Start.ToRecast().ToFloatArray().data();
    float* endPoint = End.ToRecast().ToFloatArray().data();

    dtPolyRef startPoly;
    float startPolyPoint[3];
    dtStatus startResult = navMeshQuery->findNearestPoly(startPoint, PathFinder::searchBoxSize, &Queryfilter, &startPoly, startPolyPoint);

    if (dtStatusFailed(startResult))
    {
        std::cout << "Error finding nearest poly for start point" << std::endl;
        return false;
    }

    dtPolyRef endPoly;
    float endPolyPoint[3];
    dtStatus endResult = navMeshQuery->findNearestPoly(endPoint, searchBoxSize, &Queryfilter, &endPoly, endPolyPoint);
    if (dtStatusFailed(endResult))
    {
        std::cout << "Error finding nearest poly for end point" << std::endl;
        return false;
    }

    dtStatus dtResult = DT_FAILURE;
    dtResult = navMeshQuery->findPath(
        startPoly,              // start polygon
        endPoly,                // end polygon
        startPolyPoint,         // start position
        endPolyPoint,           // end position
        &Queryfilter,           // polygon search filter
        pathPolyRefs,           // [out] path
        (int*)&polyLength,      // [out] polygon length
        MAX_PATH_LENGTH);       // max number of polygons in output path.

    if (!polyLength || dtStatusFailed(dtResult))
    {
        std::cout << "Error finding path" << std::endl;
        return false;
    }

  

    FindPath(startPoint, endPoint);
}

bool PathFinder::FindPath(const float* startPoint, const float* endPoint)
{
    dtStatus dtResult = DT_FAILURE;
    const int PATH_POINT_MAX = 100;
    float pathPointArray[PATH_POINT_MAX * 3]; 
    int pathPointCount;

    dtResult = navMeshQuery->findStraightPath(
            startPoint,
            endPoint, 
            pathPolyRefs,
            polyLength,
            pathPointArray,
            NULL,
            NULL,
            &pathPointCount,
            MAX_PATH_LENGTH);

    if (dtStatusFailed(dtResult))
    {
        std::cout << "Error finding straight path" << std::endl;
        return false;
    }


    for (int i = 0; i < pathPointCount; i++)
    {
        float* pos = &pathPointArray[i * 3];

        if (NavigationManager::DEBUGMOD)
        {
            std::cout << "x: " << pos[0] << " y: " << pos[1] << " z: " << pos[2] << std::endl;
        }
        PathFinder::pathPoints.push_back(Vector3(pos[0], pos[1], pos[2]).ToWoW());
    }
 

    return true;
}







