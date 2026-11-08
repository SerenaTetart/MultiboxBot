#ifndef MANGOS_H_MOVE_MAP_SHARED_DEFINES
#define MANGOS_H_MOVE_MAP_SHARED_DEFINES

#include <cstdint>
#include <cstddef>
#include "DetourNavMesh.h"
#include "Defines.h"

struct MmapTileHeader
{
    std::uint32_t mmapMagic;
    std::uint32_t dtVersion;
    std::uint32_t mmapVersion;
    std::uint32_t size;
    std::uint32_t usesLiquids;

    MmapTileHeader() : mmapMagic(MMAP_MAGIC), dtVersion(DT_NAVMESH_VERSION),
        mmapVersion(MMAP_VERSION), size(0), usesLiquids(0) {}
};

static_assert(sizeof(MmapTileHeader) == 20, "CMaNGOS Classic mmtile header must be 20 bytes");
static_assert(offsetof(MmapTileHeader, usesLiquids) == 16, "Unexpected mmtile header layout");

enum NavArea
{
    NAV_AREA_EMPTY = 0,
    NAV_AREA_GROUND = 11,
    NAV_AREA_GROUND_STEEP = 10,
    NAV_AREA_WATER = 9,
    NAV_AREA_MAGMA_SLIME = 8,
    NAV_AREA_MAX_VALUE = NAV_AREA_GROUND,
    NAV_AREA_MIN_VALUE = NAV_AREA_MAGMA_SLIME,
    NAV_AREA_ALL_MASK = 0x3F
};

enum NavTerrainFlag
{
    NAV_EMPTY = 0x00,
    NAV_GROUND = 1 << (NAV_AREA_MAX_VALUE - NAV_AREA_GROUND),
    NAV_GROUND_STEEP = 1 << (NAV_AREA_MAX_VALUE - NAV_AREA_GROUND_STEEP),
    NAV_WATER = 1 << (NAV_AREA_MAX_VALUE - NAV_AREA_WATER),
    NAV_MAGMA_SLIME = 1 << (NAV_AREA_MAX_VALUE - NAV_AREA_MAGMA_SLIME)
};
static_assert(NAV_GROUND == 0x01 && NAV_GROUND_STEEP == 0x02 &&
              NAV_WATER == 0x04 && NAV_MAGMA_SLIME == 0x08,
              "CMaNGOS terrain flags mismatch");

#endif
