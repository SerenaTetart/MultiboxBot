#include "Functions.h"

#include <iostream>

#include "MemoryManager.h"
#include "Game.h"
#include "FunctionsLua.h"
#include "rng.h"

#include "Navigation.h"
#include <random>

#include "DBCReader.h"

bool Functions::Intersect(Position start, Position end, float z) {
	//Need height variable because LOS is based on the position of the eyes of the char
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(start.X, start.Y, start.Z+z);
	Position p2 = Position(end.X, end.Y, end.Z+z);
	Position intersection = Position(0, 0, 0);
	float distance = float(p1.DistanceTo(p2));
	bool result = function(&p1, &p2, 0, &intersection, &distance, 0x00100111);
	return result;
}

float Functions::GetDepth(Position pos, float height) {
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(pos.X, pos.Y, pos.Z+height);
	Position p2 = Position(pos.X, pos.Y, pos.Z - 10);
	Position intersection = Position(0, 0, 0);
	float distance = float(p1.DistanceTo(p2));
	bool res = function(&p1, &p2, 0, &intersection, &distance, 0x00100111);
	if (res) distance = pos.DistanceTo(intersection);
	return distance;
}

Position Functions::ProjectPos(Position pos, float height) {
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(pos.X, pos.Y, pos.Z+height);
	Position p2 = Position(pos.X, pos.Y, pos.Z - 10);
	Position intersection = Position(0, 0, 0);
	float distance = float(p1.DistanceTo(p2));
	bool res = function(&p1, &p2, 0, &intersection, &distance, 0x00100111);
	if (res) {
		Position result = Position(pos.X, pos.Y, intersection.Z);
		return result;
	}
	else return pos;
}

unsigned long Functions::GetPlayerGuid() {
	typedef unsigned long func();
	func* function = (func*)GET_PLAYER_GUID_FUN_PTR;
	unsigned long guid = function();
	return guid;
}

int Functions::GetPositionCircle() {
	int tmp_pos = 1;
	if (localPlayer == NULL || Leader == NULL) return 1;
	for (unsigned int i = 0; i < leaderInfos.size(); i++) {
		if (Leader->name != get<0>(leaderInfos[i])) { //not Leader
			for (unsigned int z = 0; z < ListUnits.size(); z++) {
				if (ListUnits[z].name == get<0>(leaderInfos[i])) {
					if (get<0>(leaderInfos[i]) == localPlayer->name) return tmp_pos;
					tmp_pos++;
					break;
				}
			}
		}
	}
	return 1;
}

void Functions::EnumerateVisibleObjects(int filter) {
	for (int i = 0; i < 40; i++) {
		GroupMember[i] = NULL;
	}
	for (int i = 0; i < 5; i++) {
		PartyMember[i] = NULL;
	}
	ListUnits.clear();
	ListGameObjects.clear();
	if (localPlayer != NULL) {
		delete(localPlayer);
		localPlayer = NULL;
	}
	typedef int (*EnumerateVisibleObjectsCallback)(unsigned long long, int);
	EnumerateVisibleObjectsCallback callback = &Callback;
	void* callbackPtr = (void*&)callback;

	typedef void __fastcall func(uintptr_t, int);
	func* function = (func*)ENUMERATE_VISIBLE_OBJECTS_FUN_PTR;
	try {
		function((uintptr_t)callbackPtr, filter);
	}
	catch (...) {}
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		ListUnits[i].unitReaction = localPlayer->getUnitReaction(ListUnits[i].Pointer);
		ListUnits[i].attackable = localPlayer->canAttack(ListUnits[i].Pointer);
	}
	if (localPlayer != NULL) localPlayer->className = FunctionsLua::UnitClass("player");
	Leader = Functions::GetLeader();
	positionCircle = GetPositionCircle();
}

uintptr_t Functions::GetObjectPtr(unsigned long long guid) {
	typedef uintptr_t __stdcall func(unsigned long long);
	func* function = (func*)GET_OBJECT_PTR_FUN_PTR;
	uintptr_t ptr = function(guid);
	return ptr;
}

int Functions::Callback(unsigned long long guid, int filter) {
	uintptr_t pointer = Functions::GetObjectPtr(guid);
	if (pointer != 0) {
		ObjectType objectType = (*(ObjectType*)(pointer + OBJECT_TYPE_OFFSET));
		if (objectType == GameObject) {
			WoWGameObject gameObject = WoWGameObject(pointer, guid, objectType);
			ListGameObjects.push_back(gameObject);
		}
		else if (objectType == Unit || objectType == Player) {
			WoWUnit unit = WoWUnit(pointer, guid, objectType);
			ListUnits.push_back(unit);
			if (objectType == Player) {
				for (unsigned int y = 0; y < leaderInfos.size(); y++) {
					if (unit.name == get<0>(leaderInfos[y])) {
						ListUnits.back().role = get<1>(leaderInfos[y]);
						ListUnits.back().indexGroup = y;
						break;
					}
				}
				if (guid == GetPlayerGuid()) {
					localPlayer = new LocalPlayer(pointer, guid, objectType);
					localPlayer->role = ListUnits.back().role;
					localPlayer->indexGroup = ListUnits.back().indexGroup;
					GroupMember[0] = &ListUnits.back();
					PartyMember[0] = &ListUnits.back();
				}
				else if (tarType == "party") {
					for (int i = 1; i < 5; i++) {
						if (unit.name == FunctionsLua::UnitName("party" + std::to_string(i))) {
							GroupMember[i] = &ListUnits.back();
							PartyMember[i] = &ListUnits.back();
							break;
						}
					}
				}
				else {
					for (int i = 1; i <= NumGroupMembers; i++) {
						if (unit.name == FunctionsLua::UnitName("raid" + std::to_string(i))) {
							GroupMember[i] = &ListUnits.back();
							break;
						}
					}
					for (int i = 1; i < 5; i++) {
						if (unit.name == FunctionsLua::UnitName("party" + std::to_string(i))) {
							PartyMember[i] = &ListUnits.back();
							break;
						}
					}
				}
			}
		}
	}
	return 1;
}

void Functions::LuaCall(const char* code) {
	typedef void __fastcall func(const char* code, const char* unused);
	func* function = (func*)LUA_CALL_FUN_PTR;

	function(code, "Unused");
}

uintptr_t Functions::GetText(const char* varName) {
	typedef uintptr_t __fastcall func(const char* varName, unsigned int nonSense, int zero);
	func* f = (func*)LUA_GET_TEXT_FUN_PTR;
	return f(varName, -1, 0);
}

void Functions::ClickAOE(Position position) {
	float xyz[3] = { position.X, position.Y, position.Z };
	typedef void __fastcall func(float*);
	func* function = (func*)SPELL_C_HANDLETERRAINCLICK_FUN_PTR;
	function(xyz);
}

void Functions::LootUnit(uintptr_t target, int autoloot) {
	typedef void(__thiscall* func)(uintptr_t target, int autoloot);
	func function = (func)RIGHT_CLICK_UNIT_FUN_PTR;
	function(target, autoloot);
}

void Functions::InteractObject(uintptr_t object_ptr, int autoloot) {
	typedef void(__thiscall* func)(uintptr_t object_ptr, int autoloot);
	func function = (func)INTERACT_OBJECT_FUN_PTR;
	function(object_ptr, autoloot);
}

void Functions::MoveTo(Position target_pos, int MoveType, bool checkEnemyClose, bool targetSwim) {
	if ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) || targetSwim) {
		if (!Functions::Intersect(localPlayer->position, target_pos)) {
			localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
			Moving = MoveType;
		}
		else if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING) && Functions::MoveObstacle(target_pos, checkEnemyClose)) {
			Moving = MoveType;
		}
		else if ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) && Functions::MoveObstacleSwim(target_pos, checkEnemyClose)) {
			Moving = MoveType;
		}
	}
	else if (Functions::MoveObstacle(target_pos, checkEnemyClose) == false) {
		Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos);
		if (nextpos.DistanceTo(localPlayer->position) > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
			if (Functions::MoveObstacle(nextpos, checkEnemyClose) == false) {
				localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
				Moving = MoveType;
			}
			else Moving = MoveType;
		}
	} else Moving = MoveType;
}

void Functions::MoveToLoS(Position target_pos, int MoveType) {
	if ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) || (targetUnit != NULL && targetUnit->movement_flags & MOVEFLAG_SWIMMING)) {
		if (Functions::MoveLoSSwim(target_pos)) {
			Moving = MoveType;
		}
	}
	else if (Functions::MoveLoS(target_pos) == false) {
		Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos);
		if (nextpos.DistanceTo(localPlayer->position) > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
			localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
			Moving = MoveType;
		}
	}
	else Moving = MoveType;
}

void Functions::FollowMultibox(int placement) {
	if (Leader == NULL) return;
	float range = 2.0f; float cst = 0.30f;
	if (placement == 1) cst = 0.30f;
	else if (placement == 2) cst = -0.30f;
	else if (placement == 3) { range = 4.0f; cst = 0.15f; }
	else if (placement == 4) { range = 4.0f; cst = -0.15f; }
	float halfPI = acosf(0);
	Position target_pos = Position((cos(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.X
		, (sin(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.Y, Leader->position.Z);
	bool targetSwim = false; if (Leader->movement_flags & MOVEFLAG_SWIMMING) targetSwim = true;
	ThreadSynchronizer::RunOnMainThread([=]() {
		Functions::MoveTo(target_pos, 4, false, targetSwim);
	});
}

bool MoveObstacleSwim_tmp(const Position& target_pos, const Position& start_pos) {
	constexpr float STEP = 2.5f;
	constexpr int   MAX_STEPS = 16;

	float dx = target_pos.X - start_pos.X;
	float dy = target_pos.Y - start_pos.Y;
	float base = std::atan2(dy, dx);

	Position last = start_pos;

	int i = 0;
	while (i < MAX_STEPS) {
		Position stepPos(last.X + std::cos(base) * STEP, last.Y + std::sin(base) * STEP, last.Z);

		if (stepPos.DistanceTo(stepPos) > 2.0f) return false;
		if (Functions::Intersect(last, stepPos)) return false;

		if (stepPos.DistanceTo(last) < 1e-3f) return false;

		last = stepPos;
		if (last.DistanceTo(target_pos) <= STEP) return true;
		++i;
	}
	return false;
}

bool MoveObstacle_tmp(const Position& target_pos, const Position& start_pos) {
	constexpr float STEP = 2.5f;
	constexpr int   MAX_STEPS = 16;          // 16 * 2.5 = 40 yards

	float dx = target_pos.X - start_pos.X;
	float dy = target_pos.Y - start_pos.Y;
	float base = std::atan2(dy, dx); // radians

	Position last = start_pos;

	int i = 0;
	while (i < MAX_STEPS) {
		Position stepPos(last.X + std::cos(base) * STEP, last.Y + std::sin(base) * STEP, last.Z);
		Position next = Functions::ProjectPos(stepPos, 2.0f);

		// Reject if snap is too big or the segment hits something
		if (next.DistanceTo(stepPos) > 2.0f) return false;
		if (Functions::Intersect(last, next)) return false;

		// Progress guard (avoid potential stalls on weird projections)
		if (next.DistanceTo(last) < 1e-3f) return false;

		last = next;
		if (last.DistanceTo(target_pos) <= STEP) return true;
		++i;
	}
	return false;
}

bool Functions::MoveObstacleSwim(Position target_pos, bool checkEnemyClose) {
	constexpr float STEP = 2.5f;
	constexpr int   MAX_STEPS = 12;
	constexpr float ANGLE_STEP = 3.14159265358979323846f / 8.0f;
	constexpr std::array<int, 13> OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

	float dx = target_pos.X - localPlayer->position.X;
	float dy = target_pos.Y - localPlayer->position.Y;
	float base = std::atan2(dy, dx);

	for (int off : OFFSETS) {
		float dir = base + off * ANGLE_STEP;

		Position last = localPlayer->position;

		for (int s = 0; s < MAX_STEPS; ++s) {
			Position stepPos(last.X + std::cos(dir) * STEP, last.Y + std::sin(dir) * STEP, last.Z);

			if (!Functions::Intersect(localPlayer->position, stepPos) && (!checkEnemyClose || !Functions::enemyClose(stepPos, 20.0f))) {
				if (MoveObstacleSwim_tmp(target_pos, stepPos)) {
					if (off == 0) localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
					else localPlayer->ClickToMove(Move, localPlayer->Guid, stepPos);
					return true;
				}
				last = stepPos;
				continue;
			}
			break;
		}
	}

	return false;
}

bool Functions::MoveLoSSwim(Position target_pos) {
	constexpr float STEP = 2.5f;
	constexpr int   MAX_STEPS = 12;
	constexpr float ANGLE_STEP = 3.14159265358979323846f / 8.0f;
	constexpr std::array<int, 13> OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

	float dx = target_pos.X - localPlayer->position.X;
	float dy = target_pos.Y - localPlayer->position.Y;
	float base = std::atan2(dy, dx);

	for (int off : OFFSETS) {
		float dir = base + off * ANGLE_STEP;

		Position last = localPlayer->position;

		for (int s = 0; s < MAX_STEPS; ++s) {
			Position stepPos(last.X + std::cos(dir) * STEP, last.Y + std::sin(dir) * STEP, last.Z);

			if (!Functions::Intersect(last, stepPos) && !Functions::enemyClose(stepPos, 20.0f)) {
				if (!Functions::Intersect(stepPos, target_pos)) {
					localPlayer->ClickToMove(Move, localPlayer->Guid, stepPos);
					return true;
				}
				last = stepPos;
				continue;
			}
			break;
		}
	}
	return false;
}

bool Functions::MoveObstacle(Position target_pos, bool checkEnemyClose) {
	constexpr float STEP = 2.5f;                // step length in yards
	constexpr int   MAX_STEPS = 12;             // 2.5 * 12 = 30 yards max
	constexpr float ANGLE_STEP = 3.14159265358979323846f / 8.0f; // 22.5° per angular offset
	// Check straight (0), then right (+1), left (-1), then +2/-2, etc.
	constexpr std::array<int, 13> OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

	// Bearing from player to target (use atan2(-dy, dx) if Y grows downward in your coords)
	float dx = target_pos.X - localPlayer->position.X;
	float dy = target_pos.Y - localPlayer->position.Y;
	float base = std::atan2(dy, dx); // radians

	for (int off : OFFSETS) {
		float dir = base + off * ANGLE_STEP;

		Position last = localPlayer->position;

		// Walk outward from the player in 2.5-yard steps along this direction.
		for (int s = 0; s < MAX_STEPS; ++s) {
			Position stepPos(last.X + std::cos(dir) * STEP, last.Y + std::sin(dir) * STEP, last.Z);

			// Snap to navmesh/terrain/etc.
			Position next = Functions::ProjectPos(stepPos, 2.0f);

			// Acceptable snap and clear segment from 'last' -> 'next'?
			if ((next.DistanceTo(stepPos) < 2.0f) && !Functions::Intersect(localPlayer->position, next) && (!checkEnemyClose || !Functions::enemyClose(next, 20.0f))) {
				// This is the CLOSEST valid point along this ray so far.
				// If your extra constraint passes, move there.
				if (MoveObstacle_tmp(target_pos, next)) {
					if (off == 0) localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
					else localPlayer->ClickToMove(Move, localPlayer->Guid, next);
					return true;
				}
				// Keep marching outward along the same ray.
				last = next;
				continue;
			}

			// Hit an obstacle (or snap too big): give up on this direction and try next angle.
			break;
		}
	}

	// No valid point found within fan/30 yards.
	return false;
}

bool Functions::StepBack(Position target_pos, int move_type) {
	/*
		Check every directions for a position where you have line of sight of target_pos,
		if there is an obstacle on the path check a new direction
	*/
	if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) && Moving == move_type) {
		Moving = move_type;
		return true;
	}
	Position list_pos[16] = { Position(0.0f, 0.0f, 0.0f) };
	float halfPI = acosf(0);
	for (int i = 0; i < 16; i++) {
		Position last_pos = target_pos; //Take into account the difference in altitude at each point
		for (int w = 0; w < 6; w++) { //Every 2.5 yards up to 15 check for LoS point
			Position tmp_pos = Position((cos((i * halfPI / 4)) * 2.5f) + last_pos.X, (sin((i * halfPI / 4)) * 2.5f) + last_pos.Y, last_pos.Z);
			Position next_pos = tmp_pos; bool depthCheck = true;
			if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) {
				Position next_pos = Functions::ProjectPos(tmp_pos, 2.00f);
				depthCheck = (next_pos.DistanceTo(tmp_pos) < 2.00f);
			}
			if (!Functions::Intersect(last_pos, next_pos) && depthCheck) {
				if (target_pos.DistanceTo(next_pos) >= 12.0f && !Functions::Intersect(next_pos, target_pos) && !Functions::enemyClose(next_pos, 20.0f)) {
					list_pos[i] = next_pos;
				}
				else { last_pos = next_pos; continue; }
			}
			else break; //There is an obstacle on this path, we need to change
		}
	}
	// Part2: Check for the closest point to player
	float min_dist = 99999.0f; int min_dist_index = -1;
	for (int i = 0; i < 16; i++) {
		if (list_pos[i].X == 0.0f && list_pos[i].Y == 0.0f && list_pos[i].Z == 0.0f) continue;
		float dist = list_pos[i].DistanceTo(localPlayer->position);
		if (dist < min_dist) {
			min_dist_index = i;
			min_dist = dist;
		}
	}
	if (min_dist_index > -1 && min_dist > 2.0f) {
		Position candidate = Functions::RandomisePos(list_pos[min_dist_index], 3.0f, target_pos, 12.0f);
		// Position aléatoire ici
		Functions::MoveObstacle(candidate);
		if (localPlayer->className == "Mage" && candidate.DistanceTo(localPlayer->position) > 10.0f && FunctionsLua::IsSpellReady("Blink")) {
			FunctionsLua::CastSpellByName("Blink");
		}
		Moving = move_type;
		return true;
	}
	else {
		Moving = 0;
		return false;
	}
}

bool Functions::MoveLoS(Position target_pos) {
	/*
		Check every directions for a position where you have line of sight of target_pos,
		if there is an obstacle on the path check a new direction
	*/
	constexpr float STEP = 2.5f;
	constexpr int   MAX_STEPS = 12;
	constexpr float ANGLE_STEP = 3.14159265358979323846f / 8.0f;
	constexpr std::array<int, 13> OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

	float dx = target_pos.X - localPlayer->position.X;
	float dy = target_pos.Y - localPlayer->position.Y;
	float base = std::atan2(dy, dx);

	for (int off : OFFSETS) {
		float dir = base + off * ANGLE_STEP;

		Position last = localPlayer->position;

		for (int s = 0; s < MAX_STEPS; ++s) {
			Position stepPos(last.X + std::cos(dir) * STEP, last.Y + std::sin(dir) * STEP, last.Z);

			Position next = Functions::ProjectPos(stepPos, 2.0f);

			if ((next.DistanceTo(stepPos) < 2.0f) && !Functions::Intersect(last, next) && !Functions::enemyClose(next, 20.0f)) {
				if (!Functions::Intersect(next, target_pos)) {
					localPlayer->ClickToMove(Move, localPlayer->Guid, next);
					return true;
				}
				last = next;
				continue;
			}
			break;
		}
	}

	return false;
}

Position meanPos(std::vector<Position> posArr) {
	Position clusterCenter = Position(0, 0, 0);
	for (unsigned int i = 0; i < posArr.size(); i++) {
		clusterCenter.X = clusterCenter.X + posArr[i].X;
		clusterCenter.Y = clusterCenter.Y + posArr[i].Y;
		clusterCenter.Z = clusterCenter.Z + posArr[i].Z;
	}
	clusterCenter.X = clusterCenter.X / posArr.size();
	clusterCenter.Y = clusterCenter.Y / posArr.size();
	clusterCenter.Z = clusterCenter.Z / posArr.size();
	return clusterCenter;
}

unsigned int Functions::GetMapID() {
	typedef int func();
	func* function = (func*)GETMAPID_FUN_PTR;
	return function();
}

//======================================================================//
//======================   Non-memory Functions   ======================//
//======================================================================//
//(They don't use Lua calls and memory pointers)

Position Functions::RandomisePos(Position target_pos, float radius, Position away_from, float dist_away) {
	float halfPI = acosf(0);
	std::uniform_real_distribution<float> U01(0.0f, 1.0f);
	std::uniform_real_distribution<float> Uang(0.0f, halfPI * 4); // 2π

	Position candidate = target_pos;
	int NUM_TRY = 0;

	do {
		auto& rng = RNG::engine();
		float theta = Uang(rng);
		float r = std::sqrt(U01(rng)) * radius;   // r = R * sqrt(u)
		candidate.X = target_pos.X + r * std::cos(theta);
		candidate.Y = target_pos.Y + r * std::sin(theta);
		candidate.Z = target_pos.Z;
		NUM_TRY += 1;
	} while (NUM_TRY < 10 && ((dist_away > 0.0f && (candidate.DistanceTo(away_from) < dist_away)) || Functions::Intersect(target_pos, candidate) || (Functions::GetDepth(candidate, 2.0f) > 2.0f)));

	if (NUM_TRY == 10) return target_pos;
	else return candidate;
}

void Functions::ClassifyHeal() {
	//Heal all friendly players or NPC within 60 yards
	std::vector<float> PrctHp;
	HealTargetArray.clear();
	AoEHeal = 0;
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].unitReaction > Neutral) && !ListUnits[i].isdead) {
			float dist = localPlayer->position.DistanceTo(ListUnits[i].position);
			if (dist < 60) {
				PrctHp.push_back(ListUnits[i].prctHP);
				HealTargetArray.push_back(i);
			}
		}
	}
	for (int i = 0; i < 5; i++) {
		if (PartyMember[i] != NULL && PartyMember[i]->prctHP < 50) AoEHeal = AoEHeal + 1;
	}
	for (int i = PrctHp.size(); i > 0; i--) {
		for (int y = 0; y < i - 1; y++) {
			if (PrctHp[y] > PrctHp[y + 1]) {
				float tmp = PrctHp[y];
				PrctHp[y] = PrctHp[y + 1];
				PrctHp[y + 1] = tmp;
				int tmp2 = HealTargetArray[y];
				HealTargetArray[y] = HealTargetArray[y + 1];
				HealTargetArray[y + 1] = tmp2;
			}
		}
	}
}

std::tuple<Position, int> Functions::getAOETargetPos(float diameter, float max_range) {
	//Execution: ~0.05ms
	//Hierarchical Clustering implementation to find position
	std::vector<std::vector<Position>> clustersArr;
	std::vector<Position> clusters_center;
	//1- Chaque position est un cluster
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT || ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED) && ListUnits[i].attackable && (ListUnits[i].unitReaction < Neutral || (ListUnits[i].unitReaction == Neutral && !(ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED))) && ListUnits[i].speed <= 4.5f && !(ListUnits[i].flags & UNIT_FLAG_CONFUSED)) {
			std::vector<Position> cluster;
			cluster.push_back(ListUnits[i].position);
			clustersArr.push_back(cluster);
			clusters_center.push_back(ListUnits[i].position);
		}
	}
	if (clusters_center.size() > 0) {
		//2- On rassemble les clusters les + proche et distance < range/2
		int min_cluster1, min_cluster2;
		do {
			float distMin = INFINITY;
			min_cluster1 = 0; min_cluster2 = 0;
			for (unsigned int i = 0; i < clustersArr.size(); i++) {
				for (unsigned int y = 0; y < clustersArr.size(); y++) {
					if (i != y) {
						float distCluster = clusters_center[i].DistanceTo(clusters_center[y]);
						if (distCluster < distMin && distCluster < (diameter / 2)) {
							distMin = distCluster;
							min_cluster1 = i;
							min_cluster2 = y;
						}
					}
				}
			}
			if (min_cluster1 != 0 || min_cluster2 != 0) {
				//On ajoute les positions du cluster_min2 dans cluster_min1
				for (unsigned int i = 0; i < clustersArr[min_cluster2].size(); i++) {
					clustersArr[min_cluster1].push_back(clustersArr[min_cluster2][i]);
				}
				clusters_center[min_cluster1] = meanPos(clustersArr[min_cluster1]);
				//On d�truit les valeurs du cluster_min2
				clustersArr.erase(clustersArr.begin() + min_cluster2);
				clusters_center.erase(clusters_center.begin() + min_cluster2);
			}
		} while (min_cluster1 != 0 || min_cluster2 != 0);
		//3- On choisit le cluster avec le plus d'unites
		unsigned int max_cluster_unit = 0; int index = 0;
		for (unsigned int i = 0; i < clustersArr.size(); i++) {
			if (clustersArr[i].size() > max_cluster_unit && clusters_center[i].DistanceTo(localPlayer->position) < max_range) {
				max_cluster_unit = clustersArr[i].size();
				index = i;
			}
		}
		return std::make_tuple(clusters_center[index], max_cluster_unit);
	}
	else return std::make_tuple(Position(0, 0, 0), 0);
}

std::tuple<int, int, int, int> Functions::countEnemies() {
	for (int i = 0; i < 40; i++) {
		HasAggro[i].clear();
	}
	int nbr = 0, nbrClose = 0, nbrCloseFacing = 0, nbrEnemyPlayer = 0;
	float mindist = INFINITY; float mindist2 = INFINITY;
	ccTarget = NULL; PvPTarget = NULL; TankTarget = NULL;
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if (!ListUnits[i].attackable) continue;
		else if (ListUnits[i].unitReaction >= Neutral && ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED) {
			float dist = localPlayer->position.DistanceTo(ListUnits[i].position);
			if (dist < 30.0f) ccTarget = &ListUnits[i];
		}
		else if ((ListUnits[i].unitReaction < Neutral || (ListUnits[i].unitReaction == Neutral && !(ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED))) && (ListUnits[i].flags & UNIT_FLAG_IN_COMBAT || ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED)) { //Hostile
			float dist = ListUnits[i].position.DistanceTo(localPlayer->position);
			for (int y = 0; y <= NumGroupMembers; y++) {
				if ((GroupMember[y] != NULL) && (GroupMember[y]->Guid == ListUnits[i].targetGuid)) {
					HasAggro[y].push_back(&ListUnits[i]); //Check if Group Member has aggro
					if (localPlayer->role == 0 && GroupMember[y]->role != 0) {
						if (dist < mindist2) {
							mindist2 = dist;
							TankTarget = &ListUnits[i];
						}
					}
					break;
				}
			}
			if (ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED) { //Enemy player
				nbrEnemyPlayer++;
				if (dist < mindist) {
					mindist = dist;
					PvPTarget = &ListUnits[i];
				}
				if((targetUnit != NULL) && (ListUnits[i].Guid != targetUnit->Guid)) {
					float dist = localPlayer->position.DistanceTo(ListUnits[i].position);
					if (dist < 30.0f) ccTarget = &ListUnits[i]; //2nd Target for CC
				}
			}
			if(!(ListUnits[i].flags & UNIT_FLAG_CONFUSED)) nbr++;
			if (localPlayer->position.DistanceTo(ListUnits[i].position) < 7.5f) {
				if (!(ListUnits[i].flags & UNIT_FLAG_CONFUSED)) nbrClose++;
				if (localPlayer->isFacing(ListUnits[i].position, 0.5f)) {
					nbrCloseFacing++;
				}
			}
		}
	}
	return std::make_tuple(nbr, nbrClose, nbrCloseFacing, nbrEnemyPlayer);
}

bool Functions::enemyClose(Position pos, float dist) {
	//Check if there are any enemy close to position
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if (ListUnits[i].attackable && (!(ListUnits[i].movement_flags & MOVEFLAG_SWIMMING) || (localPlayer->movement_flags & MOVEFLAG_SWIMMING))
			&& !(ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && (ListUnits[i].unitReaction < Neutral) && !FactionTemplate.isNeutral(ListUnits[i].factionTemplateID)
			&& !(ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED) && ((ListUnits[i].level + 5) >= localPlayer->level) && !ListUnits[i].isdead && (ListUnits[i].position.DistanceTo(pos) < dist)) {
			return true;
		}
	}
	return false;
}

int Functions::getNbrCreatureType(int range, CreatureType type1, CreatureType type2, CreatureType type3) {
	CreatureType args[3] = { type1, type2, type3 };
	int nbr = 0;
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		float dist = localPlayer->position.DistanceTo(ListUnits[i].position);
		if (dist < range) {
			for (int y = 0; y < 3; y++) {
				if (ListUnits[i].creatureType == args[y]) nbr++;
			}
		}
	}
	return nbr;
}

bool Functions::PlayerIsRanged() {
        if(localPlayer->className == "Druid") {
                int CatFormIDs[1] = { 768 }; bool CatFormBuff = localPlayer->hasBuff(CatFormIDs, 1);
                if (CatFormBuff) return false;
                int BearFormIDs[2] = { 5487, 9634 }; bool BearFormBuff = localPlayer->hasBuff(BearFormIDs, 2);
                if (BearFormBuff) return false;
                else return true;
        }
	else if(localPlayer->className == "Mage" || localPlayer->className == "Priest" || localPlayer->className == "Warlock" || localPlayer->className == "Hunter"
		|| (localPlayer->className == "Shaman" && (playerSpec == 0 || playerSpec == 2))) return true;
	else return false;
}

WoWUnit* Functions::GetGroupDead(int mode) {
	if (mode == 0) {
		for (int i = 1; i <= NumGroupMembers; i++) {
			if ((GroupMember[i] != NULL) && GroupMember[i]->isdead) return GroupMember[i];
		}
	}
	else {
		for (int i = NumGroupMembers; i >= 1; i--) {
			if ((GroupMember[i] != NULL) && GroupMember[i]->isdead) return GroupMember[i];
		}
	}
	return NULL;
}

WoWUnit* Functions::GetLeader() {
	// Return only the first account
	if (localPlayer == NULL || leaderInfos.size() <= 1) return NULL;
	for (unsigned int z = 0; z < ListUnits.size(); z++) {
		if (!ListUnits[z].attackable && (ListUnits[z].unitReaction >= Neutral) && ListUnits[z].name == get<0>(leaderInfos[0])) {
			return &(ListUnits[z]);
		}
	}
	for (unsigned int y = 0; y < 3; y++) {
		for (unsigned int i = 1; i < leaderInfos.size(); i++) {
			if (get<1>(leaderInfos[i]) == y) {
				for (unsigned int z = 0; z < ListUnits.size(); z++) {
					if (!ListUnits[z].attackable && (ListUnits[z].unitReaction >= Neutral) && ListUnits[z].name == get<0>(leaderInfos[i])) {
						return &(ListUnits[z]);
					}
				}
			}
		}
	}
	return NULL;
}

WoWUnit* Functions::GetMissingBuff(int* IDs, int size, int hasmana, int noTank) {
	//Retourne le joueur auquel il manque le buff
	//hasmana = 0 -> everyone | hasmana = 1 -> those who have mana | hamana = 2 those who don't have mana
	for (int i = 1; i <= NumGroupMembers; i++) {
		if ((GroupMember[i] != NULL) && (GroupMember[i]->unitReaction > Neutral)
			&& (hasmana == 0 || (hasmana == 1 && GroupMember[i]->prctMana > 0) || (hasmana == 2 && GroupMember[i]->prctMana < 0))
			&& !GroupMember[i]->isdead && (localPlayer->position.DistanceTo(GroupMember[i]->position) < 40.0f)
			&& !GroupMember[i]->hasBuff(IDs, size) && !Functions::Intersect(localPlayer->position, GroupMember[i]->position)) {
			if (noTank > 0) {
				for (unsigned int y = 0; y < leaderInfos.size(); y++) {
					if (GroupMember[i]->name == get<0>(leaderInfos[y])) {
						if ((noTank == 2 && get<1>(leaderInfos[i]) > 0) || (noTank == 1 && get<1>(leaderInfos[i]) == 0)) return NULL;
						else return GroupMember[i];
					}
				}
				return NULL;
			}
			return GroupMember[i];
		}
	}
	return NULL;
}
