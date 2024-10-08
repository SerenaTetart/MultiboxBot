#ifndef Functions_H
#define Functions_H
#include <cstdint>
#include "WoWObject.h"

class Functions {
	public:
		static void pressKey(unsigned int key);
		static void releaseKey(unsigned int key);
		static bool Intersect(Position start, Position end, float height, float height2=0.0f);
		static float GetDepth(Position pos, float height);
		static Position ProjectPos(Position pos, float height);
		static unsigned long GetPlayerGuid();
		static int GetPositionCircle();
		static void EnumerateVisibleObjects(int filter);
		static uintptr_t GetObjectPtr(unsigned long long guid);
		static void LuaCall(const char* code);
		static uintptr_t GetText(const char* varName);
		static void ClickAOE(Position tpos);
		static void LootUnit(uintptr_t guid, int autoloot);
		static void InteractObject(uintptr_t object_ptr, int autoloot);
		static void MoveLoS(Position target_pos);
		static bool MoveObstacle(Position target_pos);
		static void FollowMultibox(int placement = 0);
		static unsigned int GetMapID();
		// === Non-memory Functions === //
		static void ClassifyHeal();
		static std::tuple<Position, int> getAOETargetPos(float range, float range2);
        static std::tuple<int, int, int, int> countEnemies();
        static int getNbrCreatureType(int range, CreatureType type1, CreatureType type2=Null, CreatureType type3=Null);
        static bool PlayerIsRanged();
		static WoWUnit* GetGroupDead(int mode = 0);
        static WoWUnit* GetLeader();
		static WoWUnit* GetMissingBuff(int* IDs, int size);

	private:
		const static uintptr_t OBJECT_TYPE_OFFSET = 0x14;
		const static uintptr_t GET_PLAYER_GUID_FUN_PTR = 0x00468550;
		const static uintptr_t ENUMERATE_VISIBLE_OBJECTS_FUN_PTR = 0x00468380;
		const static uintptr_t GET_OBJECT_PTR_FUN_PTR = 0x00464870;
		const static uintptr_t LUA_CALL_FUN_PTR = 0x00704CD0;
		const static uintptr_t LUA_GET_TEXT_FUN_PTR = 0x00703bf0;
		const static uintptr_t RIGHT_CLICK_UNIT_FUN_PTR = 0x60BEA0;
		const static uintptr_t SPELL_C_HANDLETERRAINCLICK_FUN_PTR = 0x006E60F0;
		const static uintptr_t INTERSECT_FUN_PTR = 0x00672170;
		const static uintptr_t GETMAPID_FUN_PTR = 0x00468580;
		const static uintptr_t INTERACT_OBJECT_FUN_PTR = 0x005F8660;

		static int Callback(unsigned long long guid, int filter);
};

#endif