#pragma once
#include "Mapper.h"
#include "Common.h"
#include <vector>
#include <string>



class NavigationManager
{
public:
	static NavigationManager* GetInstance();
	void Initialize();
	void LoadMap(unsigned int mapId);
	void Release();

	void FreePathArr(Vector3* pathArr);

	Vector3* CalculatePath(unsigned int mapId, Vector3 start, Vector3 end , int* length);
	static bool DEBUGMOD;

private:

	static NavigationManager* s_singletonInstance;
	void InitializeMaps(Mapper* manager, unsigned int mapId);

};