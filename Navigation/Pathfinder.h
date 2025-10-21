#pragma once
#include <vector>
#include <DetourNavMeshQuery.h>
#include "NavigationManager.h"
#include "Defines.h"
#include "Marker.h"



class PathFinder
{
public:

	PathFinder(unsigned int mapId, unsigned int instanceId);
	bool calculate(float originX, float originY, float originZ, float destX, float destY, float destZ);
	bool FindPolyPath(Vector3 Start, Vector3 End);
	bool FindPath(const float* startPoint, const float* endPoint);
	std::vector<Vector3>& getPath() { return pathPoints; }
    static bool ApplyCircleBlacklistToPolys(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query, Marker Options);
	dtStatus DistanceToWall(float* pos, float* Hitpos, float* distance);
	bool ModifyPoint(float* Pos);
	std::vector<Vector3> ChaikinCurve(const std::vector<Vector3>& points, int iterations);


private:

	void SetFilters();
	

    dtQueryFilter Queryfilter;
	float searchBoxSize[VERTEX_SIZE] = { 3.0f, 5.0f, 3.0f };

	dtPolyRef  GetNearestPoly(const float* startPoint) const;

	bool inRangeYZX(const float* v1, const float* v2, float r, float h) const;
	bool inRange(const Vector3& p1, const Vector3& p2, float r, float h) const;
	float dist3DSqr(const Vector3& p1, const Vector3& p2) const;





	void clear()
	{
		polyLength = 0;
		pathPoints.clear();
	}

	/**
	 * @brief Query filter used for navigation queries.
	 */
	//dtQueryFilter Queryfilter;

	/**
	 * @brief Array to store maximum polygon counts.
	 *
	 * @param MAX_PATH_LENGTH The maximum length of the path.
	 */
	dtPolyRef pathPolyRefs[MAX_PATH_LENGTH];


	std::vector<Vector3>  pathPoints;

	/**
	 * @brief Length of the polygon array.
	 */
	unsigned int polyLength;

	/**
	 * @brief Checks if a tile is present at the specified position.
	 *
	 * @param p The position to check.
	 * @return True if a tile is present at the specified position, false otherwise.
	 */
	bool HaveTile(const Vector3& p) const;

	/**
	 * @brief Pointer to the navigation mesh.
	 */
	const dtNavMesh* navMesh;

	/**
	 * @brief Pointer to the navigation mesh query.
	 */
	dtNavMeshQuery* navMeshQuery;

	bool useStraightPath;  // type of path will be generated
	bool forceDestination; // when set, we will always arrive at given point
	unsigned int pointPathLimit;   // limit point path size; min(this, MAX_POINT_PATH_LENGTH)

	Vector3 startPosition;    // {x, y, z} of current location
	Vector3 endPosition;      // {x, y, z} of the destination
	Vector3 actualEndPosition;// {x, y, z} of the closest possible point to given destination

	const unsigned int  mapId;       // map id
	const unsigned int  InstanceId;       // instance id



};


