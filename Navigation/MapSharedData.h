#ifndef MANGOS_H_MOVE_MAP_SHARED_DEFINES
#define MANGOS_H_MOVE_MAP_SHARED_DEFINES

#include "DetourNavMesh.h"
#include "Defines.h"



/**
* @brief Header structure for MMAP tiles.
*/
struct MmapTileHeader
{
    unsigned int mmapMagic; ///< Magic number identifying MMAP files.
    unsigned int dtVersion; ///< Detour version number.
    unsigned int mmapVersion; ///< Version number of the MMAP format.
    unsigned int size; ///< Size of the tile.
    bool usesLiquids : 1; ///< Flag indicating if the tile uses liquids.

    /**
     * @brief Default constructor for MmapTileHeader.
     * Initializes fields with default values.
     */
    MmapTileHeader() : mmapMagic(MMAP_MAGIC), dtVersion(DT_NAVMESH_VERSION),
        mmapVersion(MMAP_VERSION), size(0), usesLiquids(false) {} //usesLiquids(true) {} //Remove liquid in paths (not 100% with current maps)
};

/**
 * @brief Enum representing types of navigation terrain.
 */
enum NavTerrain
{
    NAV_EMPTY = 0x00, ///< Empty terrain.
    NAV_GROUND = 0x01, ///< Ground terrain.
    NAV_MAGMA = 0x02, ///< Magma terrain.
    NAV_SLIME = 0x04, ///< Slime terrain.
    NAV_WATER = 0x08, ///< Water terrain.
    NAV_UNUSED1 = 0x10, ///< Unused terrain type 1.
    NAV_UNUSED2 = 0x20, ///< Unused terrain type 2.
    NAV_UNUSED3 = 0x40, ///< Unused terrain type 3.
    NAV_UNUSED4 = 0x80 ///< Unused terrain type 4.
    // we only have 8 bits
};


#endif 