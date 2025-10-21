#pragma once

#include <map>
#include <unordered_map>
#include <iostream>
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"


inline void* dtCustomAlloc(size_t size, dtAllocHint /*hint*/)
{
	return (void*)new unsigned char[size];
}

inline void dtCustomFree(void* ptr)
{
	delete[](unsigned char*)ptr;
}

typedef std::unordered_map<unsigned int, dtTileRef> MMapTileSet;
typedef std::unordered_map<unsigned int, dtNavMeshQuery*> NavMeshQuerySet;

struct MMapData
{
	MMapData(dtNavMesh* mesh) : navMesh(mesh) {}
	~MMapData()
	{
		for (NavMeshQuerySet::iterator i = navMeshQueries.begin(); i != navMeshQueries.end(); ++i)
		{
			dtFreeNavMeshQuery(i->second);
		}

		if (navMesh)
		{
			dtFreeNavMesh(navMesh);
		}
	}

	dtNavMesh* navMesh;

	// we have to use single dtNavMeshQuery for every instance, since those are not thread safe
	NavMeshQuerySet navMeshQueries;     // instanceId to query
	MMapTileSet mmapLoadedTiles;        // maps [map grid coords] to [dtTile]
};

typedef std::unordered_map<unsigned int, MMapData*> MMapDataSet;
class Mapper
{
public:
	~Mapper();



	/**
	 * @brief Map containing information about whether each zone is loaded.
	 *
	 * Key: Map ID
	 * Value: Boolean indicating whether the map is loaded or not.
	 */
	std::map<unsigned int, bool> zoneMap = {};

    /**
     * @brief Get the name of the tile at the specified coordinates in the given map.
     *
     * @param mapId The ID of the map.
     * @param x The X-coordinate of the tile.
     * @param y The Y-coordinate of the tile.
     * @param[out] result Reference to a string where the result will be stored.
     */
    static void getTileName(unsigned int mapId, int x, int y, std::string& result);


    /**
     * @brief Get the name of the map with the specified ID.
     *
     * @param mapId The ID of the map.
     * @param[out] result Reference to a string where the result will be stored.
     */
    static void getMapName(unsigned int mapId, std::string& result);

		/**
	 * @brief Loads a map at the specified tile coordinates.
	 *
	 * @param mapId The ID of the map to load.
	 * @param x The x-coordinate of the tile.
	 * @param y The y-coordinate of the tile.
	 * @return True if the map was successfully loaded, false otherwise.
	 */
	bool loadMap(unsigned int mapId, int x, int y);

	dtNavMeshQuery* GetNavMeshQuery(unsigned int mapId, unsigned int instanceId);
	dtNavMesh const*  GetNavMesh(unsigned int mapId);
	MMapDataSet loadedMMaps;

private:

	/**
   * @brief Converts a number to a string.
   *
   * @tparam T The type of the number to convert.
   * @param Number The number to convert to a string.
   * @return The string representation of the number.
   */
	template <typename T>
	static std::string NumberToString(T Number);

	/**
	 * @brief Replaces all occurrences of a substring in a string.
	 *
	 * @param s The original string.
	 * @param search The substring to search for.
	 * @param replace The string to replace the occurrences with.
	 */
	static void str_replace(std::string& s, const std::string& search, const std::string& replace);

	/**
	 * @brief Loads map data for a given map ID.
	 *
	 * @param mapId The ID of the map to load.
	 * @return True if the map data was successfully loaded, false otherwise.
	 */
	bool loadMapData(unsigned int mapId);

	/**
	 * @brief Packs tile coordinates into a single unsigned integer.
	 *
	 * @param x The x-coordinate of the tile.
	 * @param y The y-coordinate of the tile.
	 * @return The packed tile ID.
	 */
	unsigned int packTileID(int x, int y);

	/**
	 * @brief Map containing information about whether each zone is loaded.
	 *
	 * Key: Map ID
	 * Value: Boolean indicating whether the map is loaded or not.
	 */
	/*MMapDataSet loadedMMaps;*/

};

class MapperHandle
{
public:
	static Mapper* MapHandle();

};


