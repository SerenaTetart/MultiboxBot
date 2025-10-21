#pragma once
#include "DetourNavMeshQuery.h"
#include <DetourCommon.h>
#include "Defines.h"
#include <map>
#include <vector>
#include "Vector3.h"


class NavivationFilter : public dtQueryFilter
{
public:
    NavivationFilter() : dtQueryFilter() {}


    bool passFilter(const dtPolyRef polyRef, const dtMeshTile* tile, const dtPoly* poly) const override;
    static bool IsTileInBlacklistArea(const dtMeshTile* tile, const std::array<float, 3>& blacklistPosition, float blacklistRadius);



};