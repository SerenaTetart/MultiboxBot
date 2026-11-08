#include "Mapper.h"
#include "File.h"
#include <set>
#include <sstream>
#include <fstream>
#include "MapSharedData.h"


static_assert(sizeof(dtPolyRef) == 8 && sizeof(dtTileRef) == 8,
              "CMaNGOS Classic MMAP v8 requires DT_POLYREF64");
static_assert(DT_SALT_BITS == 12 && DT_TILE_BITS == 21 && DT_POLY_BITS == 31,
              "Detour polygon reference layout must match CMaNGOS Classic");

Mapper::~Mapper()
{
	for (MMapDataSet::iterator i = loadedMMaps.begin(); i != loadedMMaps.end(); ++i)
	{
		delete i->second;
	}
}


template <typename T>
std::string Mapper::NumberToString(T Number)
{
	std::ostringstream ss;
	ss << Number;
	return ss.str();
}

void Mapper::str_replace(std::string& s, const std::string& search, const std::string& replace)
{
	for (size_t pos = 0;; pos += replace.length())
	{
		pos = s.find(search, pos);
		if (pos == std::string::npos) break;

		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}


void Mapper::getTileName(unsigned int mapId, int x, int y, std::string& result)
{
	std::string tileName = "";
	if (mapId < 10)
	{
		tileName = tileName.append("00");
	}
	else if (mapId < 100)
	{
		tileName = tileName.append("0");
	}
	tileName.append(NumberToString(mapId));

	if (x < 10)
	{
		tileName = tileName.append("0");
	}
	tileName.append(NumberToString(x));

	if (y < 10)
	{
		tileName = tileName.append("0");
	}
	tileName.append(NumberToString(y));

	tileName.append(".mmtile");

	std::string pathToMmapFile = FileManager::MapPath; 

	pathToMmapFile.append(tileName);


	str_replace(pathToMmapFile, "\\", "\\\\");

	result = pathToMmapFile;



}


void Mapper::getMapName(unsigned int mapId, std::string& result)
{
    std::string mapIdStr = "";
    if (mapId < 10)
    {
        mapIdStr.append("00");
    }
    else if (mapId < 100)
    {
        mapIdStr.append("0");
    }
    mapIdStr.append(Mapper::NumberToString(mapId));

	
	mapIdStr.append(".mmap");

    std::string pathToMmapFile = FileManager::MapPath; 

    pathToMmapFile.append(mapIdStr);

    str_replace(pathToMmapFile, "\\", "\\\\");

    result = pathToMmapFile;
}

bool Mapper::loadMapData(unsigned int mapId)
{
    if (loadedMMaps.find(mapId) != loadedMMaps.end())
        return true;

    std::string filename;
    getMapName(mapId, filename);
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file)
        return false;

    dtNavMeshParams params{};
    const size_t count = fread(&params, sizeof(params), 1, file);
    fclose(file);
    if (count != 1)
        return false;

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh)
        return false;
    const dtStatus status = mesh->init(&params);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(mesh);
        return false;
    }
    loadedMMaps.emplace(mapId, new MMapData(mesh));
    return true;
}

bool Mapper::loadMap(unsigned int mapId, int x, int y)
{
    if (!loadMapData(mapId))
        return false;
    MMapData* mmap = loadedMMaps.at(mapId);
    const unsigned int key = packTileID(x, y);
    if (mmap->mmapLoadedTiles.find(key) != mmap->mmapLoadedTiles.end())
        return true;

    std::string filename;
    getTileName(mapId, x, y, filename);
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file)
        return false;

    MmapTileHeader header;
    const size_t headerRead = fread(&header, sizeof(header), 1, file);
    if (headerRead != 1 || header.mmapMagic != MMAP_MAGIC ||
        header.mmapVersion != MMAP_VERSION || header.dtVersion != DT_NAVMESH_VERSION ||
        header.size == 0)
    {
        fclose(file);
        return false;
    }

    const long payloadBegin = ftell(file);
    if (payloadBegin < 0 || fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }
    const long fileEnd = ftell(file);
    if (fileEnd < payloadBegin ||
        static_cast<unsigned long long>(fileEnd - payloadBegin) != header.size ||
        fseek(file, payloadBegin, SEEK_SET) != 0)
    {
        fclose(file);
        return false;
    }

    unsigned char* data = static_cast<unsigned char*>(dtAlloc(header.size, DT_ALLOC_PERM));
    if (!data)
    {
        fclose(file);
        return false;
    }
    const size_t tileRead = fread(data, header.size, 1, file);
    fclose(file);
    if (tileRead != 1)
    {
        dtFree(data);
        return false;
    }

    dtTileRef tileRef = 0;
    const dtStatus status = mmap->navMesh->addTile(data, header.size, DT_TILE_FREE_DATA, 0, &tileRef);
    if (dtStatusFailed(status))
    {
        dtFree(data);
        return false;
    }
    mmap->mmapLoadedTiles.emplace(key, tileRef);
    return true;
}

unsigned int Mapper::packTileID(int x, int y)
{
	return (static_cast<unsigned int>(x) << 16) | static_cast<unsigned int>(y);
}

dtNavMeshQuery* Mapper::GetNavMeshQuery(unsigned int mapId, unsigned int instanceId)
{
    auto it = loadedMMaps.find(mapId);
    if (it == loadedMMaps.end())
        return nullptr;

    MMapData* mmap = it->second;
    auto queryIt = mmap->navMeshQueries.find(instanceId);
    if (queryIt != mmap->navMeshQueries.end())
        return queryIt->second;

    dtNavMeshQuery* query = dtAllocNavMeshQuery();
    if (!query)
        return nullptr;
    const dtStatus status = query->init(mmap->navMesh, 65535);
    if (dtStatusFailed(status))
    {
        dtFreeNavMeshQuery(query);
        return nullptr;
    }
    mmap->navMeshQueries.emplace(instanceId, query);
    return query;
}

dtNavMesh const* Mapper::GetNavMesh(unsigned int mapId)
{
    auto it = loadedMMaps.find(mapId);
    return it == loadedMMaps.end() ? nullptr : it->second->navMesh;
}

Mapper* Handle = NULL;

Mapper* MapperHandle::MapHandle()
{
	if (Handle == NULL)
		Handle = new Mapper();

	return Handle;
}
