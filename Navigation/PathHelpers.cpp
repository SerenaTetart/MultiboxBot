#include "Pathfinder.h"
#include "Marker.h"
#include "AreaEnums.h"


bool PathFinder::ApplyCircleBlacklistToPolys(dtNavMeshQuery* meshQuery, const dtNavMesh* navmesh, dtQueryFilter query, Marker Options)
{
    if (!meshQuery || !navmesh)
    {
        std::cerr << "MeshQuery or NavMesh is null." << std::endl;
        return false;
    }
    float searchBoxSize[VERTEX_SIZE] = { 3.0f, 5.0f, 3.0f };
    float* searchPoint = Options.position.ToRecast().ToFloatArray().data();

    dtPolyRef CenterPoly;
    float CenterPolyPoint[3];
    dtStatus startResult = meshQuery->findNearestPoly(searchPoint, searchBoxSize, &query, &CenterPoly, CenterPolyPoint);
    if (dtStatusFailed(startResult)) {
        std::cerr << "Failed to find nearest poly." << std::endl;
        return false;
    }

    const int POLY_ARRAY_MAX = 1000;
    dtPolyRef polyRefsInCircle[POLY_ARRAY_MAX];
    int polyRefInCircleCount = 0;

    dtStatus circleResult = meshQuery->findPolysAroundCircle(CenterPoly, searchPoint, Options.radius, &query, polyRefsInCircle, nullptr, nullptr, &polyRefInCircleCount, POLY_ARRAY_MAX);
    if (dtStatusFailed(circleResult)) {
        std::cerr << "Failed to find polys around circle." << std::endl;
        return false;
    }

    for (int i = 0; i < polyRefInCircleCount; ++i) {
        dtPolyRef polyRef = polyRefsInCircle[i];

        const dtMeshTile* tile = nullptr;
        const dtPoly* poly = nullptr;

        dtStatus tileAndPolyStatus = navmesh->getTileAndPolyByRef(polyRef, &tile, &poly);
        if (dtStatusFailed(tileAndPolyStatus) || !poly)
        {
            std::cerr << "Error retrieving tile and polygon by ref." << std::endl;
            continue;
        }

        // Modify the polygon area directly by using mutablePoly
        dtPoly* mutablePoly = const_cast<dtPoly*>(poly);

        if (NavigationManager::DEBUGMOD)
        {
            std::cout << "Polygon area: " << static_cast<int>(mutablePoly->getArea()) << std::endl;
        }

        // Example: Set area ID to 35 (or any specific value you want)
        mutablePoly->setArea(Options.Type);

        if (NavigationManager::DEBUGMOD)
        {
            // Debug output: Print the updated area of the mutablePoly
            std::cout << "Polygon area after setting: " << static_cast<int>(mutablePoly->getArea()) << std::endl;
        }

      
    }

    return true;
}


void PathFinder::SetFilters()
{
	//Queryfilter.setAreaCost(Area::Walkable, 1);
	Queryfilter.setAreaCost(Area::LowThread, 100);
	Queryfilter.setAreaCost(Area::HighThread, 150);
	Queryfilter.setAreaCost(Area::Blacklisted, 1000);

	unsigned short includeFlags = 0;
	unsigned short excludeFlags = 0;
	includeFlags |= (0x01);

	Queryfilter.setIncludeFlags(includeFlags);
	Queryfilter.setExcludeFlags(excludeFlags);

}

bool PathFinder::HaveTile(const Vector3& p) const
{
	int tx, ty;
	float point[3] = { p.Y, p.Z, p.X };

	navMesh->calcTileLoc(point, &tx, &ty);
	auto r = navMesh->getTileAt(tx, ty, 0);
	

	if (r == NULL)
	{
		std::cout << "Tile Failed" << std::endl;
	}

	return (navMesh->getTileAt(tx, ty, 0) != NULL);
}



bool PathFinder::inRangeYZX(const float* v1, const float* v2, float r, float h) const
{
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1]; // elevation
	const float dz = v2[2] - v1[2];
	return (dx * dx + dz * dz) < r * r && fabsf(dy) < h;
}

bool PathFinder::inRange(const Vector3& p1, const Vector3& p2, float r, float h) const
{
	Vector3 d = p1 - p2;
	return (d.X * d.X + d.Y * d.Y) < r * r && fabsf(d.Z) < h;
}
//
float PathFinder::dist3DSqr(const Vector3& p1, const Vector3& p2) const
{
	return (p1 - p2).squaredLength();
}

dtPolyRef PathFinder::GetNearestPoly(const float* startPoint) const
{
	dtPolyRef startPoly;
	float startPolyPoint[3];
	dtStatus results =  navMeshQuery->findNearestPoly(startPoint, PathFinder::searchBoxSize, &Queryfilter, &startPoly, startPolyPoint);
	if (dtStatusFailed(results))
	{
		std::cout << "Failed to GetNearestPoly" << std::endl;
		return NULL;
	}
	return startPoly;
}


dtStatus PathFinder::DistanceToWall(float* pos, float* Hitpos, float* distance)
{

    dtPolyRef polyref;
    float CenterPolyPoint[3];
    float hitNormal[3];

    dtStatus startResult = navMeshQuery->findNearestPoly(pos, searchBoxSize, &Queryfilter, &polyref, CenterPolyPoint);

    if (dtStatusFailed(startResult))
    {
        std::cerr << "CheckPath : Failed to find nearest poly." << std::endl;
        return false;
    }

    return navMeshQuery->findDistanceToWall(
        polyref,                   // [in]    startRef The reference id of the polygon containing @p centerPos.
        pos,                   // [in]    centerPos The center of the search circle.[(x, y, z)]
        100.0f,                // [in]    maxRadius The radius of the search circle.
        &Queryfilter,          // [in]    queryFilter The query filter used to find the nearest wall.
        distance,              // [out]   hitDist The distance to the nearest wall from @p centerPos.
        Hitpos,                // [out]   hitPos The nearest position on the wall that was hit. [(x, y, z)]
        hitNormal);            // [out]   hitNormal The normalized ray formed from the wall point to the hit point.
}

#define WallDistance 1.5f

bool PathFinder::ModifyPoint(float* SearchPoint)
{

    float hitpos[3];
    float DistanceLeft = 0.0f;
    float DistanceRight = 0.0f;
    float DistanceFront = 0.0f;

    // Calculate the position to the left
    float leftPosition[3] = { SearchPoint[0] - WallDistance, SearchPoint[1], SearchPoint[2] };
    dtStatus Left = DistanceToWall(leftPosition, hitpos, &DistanceLeft);
    if (dtStatusFailed(Left))
    {
        std::cerr << "CheckPath : Failed to find distance to wall on the left." << std::endl;
        return false;
    }

    std::cout << "Left Position: x= " << leftPosition[0] << ", y= " << leftPosition[1] << ", z= " << leftPosition[2] << "\n";
    std::cout << "Left Hitpos: x= " << hitpos[0] << ", y= " << hitpos[1] << ", z= " << hitpos[2] << "\n";
    std::cout << "DistanceLeft: " << DistanceLeft << "\n\n";

    // Calculate the position to the right
    float rightPosition[3] = { SearchPoint[0] + WallDistance, SearchPoint[1], SearchPoint[2] };
    dtStatus Right = DistanceToWall(rightPosition, hitpos, &DistanceRight);
    if (dtStatusFailed(Right))
    {
        std::cerr << "CheckPath : Failed to find distance to wall on the right." << std::endl;
        return false;
    }

    std::cout << "Right Position: x= " << rightPosition[0] << ", y= " << rightPosition[1] << ", z= " << rightPosition[2] << "\n";
    std::cout << "Right Hitpos: x= " << hitpos[0] << ", y= " << hitpos[1] << ", z= " << hitpos[2] << "\n";
    std::cout << "DistanceRight: " << DistanceRight << "\n\n";

    // Calculate the position in front
    float frontPosition[3] = { SearchPoint[0], SearchPoint[1], SearchPoint[2] + WallDistance };
    dtStatus Front = DistanceToWall(frontPosition, hitpos, &DistanceFront);
    if (dtStatusFailed(Front))
    {
        std::cerr << "CheckPath : Failed to find distance to wall in front." << std::endl;
        return false;
    }

    std::cout << "Front Position: x= " << frontPosition[0] << ", y= " << frontPosition[1] << ", z= " << frontPosition[2] << "\n";
    std::cout << "Front Hitpos: x= " << hitpos[0] << ", y= " << hitpos[1] << ", z= " << hitpos[2] << "\n";
    std::cout << "DistanceFront: " << DistanceFront << "\n\n";

    return false;
}


std::vector<Vector3> PathFinder::ChaikinCurve(const std::vector<Vector3>& points, int iterations)
{
    std::vector<Vector3> result = points;

    for (int it = 0; it < iterations; ++it) {
        std::vector<Vector3> newPoints;

        for (size_t i = 0; i < result.size() - 1; ++i)
        {
            Vector3 p0 = result[i];
            Vector3 p1 = result[i + 1];

            Vector3 Q = p0 + 0.75f * (p1 - p0);
            Vector3 R = p0 + 0.25f * (p1 - p0);

            newPoints.push_back(R);
            newPoints.push_back(Q);
        }

        // Include the last point to close the curve if necessary
        if (!result.empty())
        {
            newPoints.push_back(result.back());
        }

        result = std::move(newPoints);
    }

    return result;
}

