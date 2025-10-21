#include "Mapper.h"
#include "File.h"
#include <set>
#include <sstream>
#include <fstream>
#include "MapSharedData.h"


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

	std::string fileName = "";
	getMapName(mapId, fileName);
	FILE* file = fopen(fileName.c_str(), "rb");
	dtNavMeshParams params;
	size_t file_read = fread(&params, sizeof(dtNavMeshParams), 1, file);
	fclose(file);

	dtNavMesh* mesh = dtAllocNavMesh();
	
	dtStatus dtResult = mesh->init(&params);
	if (dtStatusFailed(dtResult))
	{
		dtFreeNavMesh(mesh);
		return false;

	}
	MMapData* mmap_data = new MMapData(mesh);
	mmap_data->mmapLoadedTiles.clear();
	loadedMMaps.insert(std::pair<unsigned int, MMapData*>(mapId, mmap_data));

	return true;
}

bool Mapper::loadMap(unsigned int mapId, int x, int y)
{
	loadMapData(mapId);

	MMapData* mmap = loadedMMaps[mapId];

	unsigned int packedGridPos = packTileID(x, y);
	if (mmap->mmapLoadedTiles.find(packedGridPos) != mmap->mmapLoadedTiles.end())
		return true;

	std::string fileName = "";
	getTileName(mapId, x, y, fileName);


	FILE* file = fopen(fileName.c_str(), "rb");
	MmapTileHeader fileHeader;
	size_t file_read = fread(&fileHeader, sizeof(MmapTileHeader), 1, file);
	unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
	size_t result = fread(data, fileHeader.size, 1, file);
	fclose(file);

	dtMeshHeader* header = (dtMeshHeader*)data;
	dtTileRef tileRef = 0;
	dtStatus dtResult = mmap->navMesh->addTile(data, fileHeader.size, DT_TILE_FREE_DATA, 0, &tileRef);

	if (dtStatusFailed(dtResult))
	{
		dtFree(data);
		return false;
	}

	mmap->mmapLoadedTiles.insert(std::pair<unsigned int, dtTileRef>(packedGridPos, tileRef));
	return false;
}

unsigned int Mapper::packTileID(int x, int y)
{
	return unsigned int(x << 16 | y);
}

dtNavMeshQuery* Mapper::GetNavMeshQuery(unsigned int mapId, unsigned int instanceId)
{
	if (loadedMMaps.find(mapId) == loadedMMaps.end()) // Map is there aka loaded in memory
		return NULL;

	MMapData* mmap = loadedMMaps[mapId];
	if (mmap->navMeshQueries.find(instanceId) == mmap->navMeshQueries.end())
	{
		dtNavMeshQuery* query = dtAllocNavMeshQuery();
		dtStatus dtResult = query->init(mmap->navMesh, 65535);
		if (dtStatusFailed(dtResult))
		{
			std::cout << "error GetNavMeshQuery" << std::endl;
			dtFreeNavMeshQuery(query);
			return NULL;
		}

		mmap->navMeshQueries.insert(std::pair<unsigned int, dtNavMeshQuery*>(instanceId, query));
	}

	return mmap->navMeshQueries[instanceId];
}
dtNavMesh const* Mapper::GetNavMesh(unsigned int mapId)
{
	if (loadedMMaps.find(mapId) == loadedMMaps.end())
	{
		std::cout << "error  GetNavMesh" << std::endl;
		return NULL;
	}

	return loadedMMaps[mapId]->navMesh;
}

Mapper* Handle = NULL;

Mapper* MapperHandle::MapHandle()
{
	if (Handle == NULL)
		Handle = new Mapper();

	return Handle;
}
