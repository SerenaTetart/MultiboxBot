#include "Functions.h"

#include <iostream>

#include "MemoryManager.h"
#include "Game.h"

// Use them OUTSIDE main thread !
void Functions::pressKey(unsigned int key) {
	SendMessageW(ThreadSynchronizer::windowHandle, WM_KEYDOWN, key, 0);
}

void Functions::releaseKey(unsigned int key) {
	SendMessageW(ThreadSynchronizer::windowHandle, WM_KEYUP, key, 0);
}

bool Functions::Intersect(Position start, Position end, float height, float height2) {
	//Need height variable because LOS is based on the position of the eyes of the char
	if (height2 == 0.0f) height2 = height;
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(start.X, start.Y, start.Z+height);
	Position p2 = Position(end.X, end.Y, end.Z+height2);
	Position intersection = Position(0, 0, 0);
	float distance = float(start.DistanceTo(end));
	bool result = function(&p1, &p2, 0, &intersection, &distance, 0x00100111);
	return result;
}

float Functions::GetDepth(Position pos, float height) {
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(pos.X, pos.Y, pos.Z+height);
	Position p2 = Position(pos.X, pos.Y, pos.Z - 100);
	Position intersection = Position(0, 0, 0);
	float distance = float(pos.DistanceTo(p2));
	bool res = function(&p1, &p2, 0, &intersection, &distance, 0x00100111);
	float result = pos.DistanceTo(intersection);
	return result;
}

Position Functions::ProjectPos(Position pos, float height) {
	typedef bool __fastcall func(Position* p1, Position* p2, int ignore, Position* intersection, float* distance, unsigned int flags);
	func* function = (func*)INTERSECT_FUN_PTR;
	Position p1 = Position(pos.X, pos.Y, pos.Z+height);
	Position p2 = Position(pos.X, pos.Y, pos.Z - 100);
	Position intersection = Position(0, 0, 0);
	float distance = float(pos.DistanceTo(p2));
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

void GetLeader() {
	for (unsigned int y = 0; y < 2; y++) {
		for (unsigned int i = 0; i < leaderInfos.size(); i++) {
			if (get<1>(leaderInfos[i]) == y && get<2>(leaderInfos[i]) == 1) {
				for (unsigned int z = 0; z < ListUnits.size(); z++) {
					if (ListUnits[z].name == get<0>(leaderInfos[i]) && localPlayer->position.DistanceTo(ListUnits[z].position) < 60.0f) {
						Leader = &(ListUnits[z]);
						return;
					}
				}
			}
		}
	}
}

void Functions::EnumerateVisibleObjects(int filter) {
	for (int i = 0; i < 40; i++) GroupMember[i] = NULL;
	ListUnits.clear();
	for (unsigned int i = 0; i < leaderInfos.size(); i++) get<2>(leaderInfos[i]) = 0;
	typedef int (*EnumerateVisibleObjectsCallback)(unsigned long long, int);
	EnumerateVisibleObjectsCallback callback = &Callback;
	void* callbackPtr = (void*&)callback;

	typedef void __fastcall func(uintptr_t, int);
	func* function = (func*)ENUMERATE_VISIBLE_OBJECTS_FUN_PTR;
	try {
		function((uintptr_t)callbackPtr, filter);
	}
	catch (...) {}

	GetLeader();

	int tmp_pos = 1;
	for (unsigned int i = 0; i < leaderInfos.size(); i++) {
		if (localPlayer == NULL || Leader == NULL) break;
		if (get<2>(leaderInfos[i]) == 1 && Leader != NULL && Leader->name != get<0>(leaderInfos[i])) { //Visible and not Leader
			if (get<0>(leaderInfos[i]) == localPlayer->name) {
				positionCircle = tmp_pos;
				break;
			}
			tmp_pos++;
		}
	}

	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		ListUnits[i].unitReaction = localPlayer->getUnitReaction(ListUnits[i].Pointer);
		ListUnits[i].attackable = localPlayer->canAttack(ListUnits[i].Pointer);
	}
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
		if (objectType == Unit || objectType == Player) {
			WoWUnit unit = WoWUnit(pointer, guid, objectType);
			ListUnits.push_back(unit);
			if (objectType == Player) {
				for (unsigned int y = 0; y < leaderInfos.size(); y++) {
					if (unit.name == get<0>(leaderInfos[y])) {
						get<2>(leaderInfos[y]) = 1;
						break;
					}
				}
				if (guid == GetPlayerGuid()) {
					if (localPlayer != NULL) {
						delete(localPlayer);
						localPlayer = NULL;
					}
					localPlayer = new LocalPlayer(pointer, guid, objectType);
					GroupMember[0] = &ListUnits.back();
				}
				else {
					for (int i = 1; i <= NumGroupMembers; i++) {
						if (unit.name == Functions::UnitName(tarType + std::to_string(i))) {
							GroupMember[i] = &ListUnits.back();
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
				if (ListUnits[i].objectType == Player && ListUnits[i].prctHP < 60) AoEHeal = AoEHeal + 1;
				HealTargetArray.push_back(i);
			}
		}
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

void Functions::FollowMultibox(int placement) {
	if (Leader == NULL) return;
	float range = 2.0f; float cst = 0.30f;
	if (placement == 1) cst = 0.30f;
	else if (placement == 2) cst = -0.30f;
	else if (placement == 3) { range = 4.0f; cst = 0.15f; }
	else if (placement == 4) { range = 4.0f; cst = -0.15f; }
	float dist = localPlayer->position.DistanceTo(Leader->position);
	if (dist < 60.0f && dist > range + 1) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			Position target_pos = Position((cos(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.X
				, (sin(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.Y, Leader->position.Z);
			bool swimming = ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) || (localPlayer->movement_flags & MOVEFLAG_WATERWALKING)
				|| (Leader->movement_flags & MOVEFLAG_SWIMMING) || (Leader->movement_flags & MOVEFLAG_WATERWALKING));
			if (swimming) localPlayer->ClickToMove(Move, Leader->Guid, target_pos);
			else {
				target_pos = Functions::ProjectPos(target_pos, 2.00f);
				Functions::MoveObstacle(target_pos);
			}
		});
	}
}

bool Functions::EnemyClose(Position pos) {
	for (unsigned int y = 0; y < ListUnits.size(); y++) {
		if ((targetUnit == NULL || (ListUnits[y].Guid != targetUnit->Guid)) && ListUnits[y].attackable && (ListUnits[y].unitReaction <= Hostile)
			&& !ListUnits[y].isdead && !(ListUnits[y].flags & UNIT_FLAG_IN_COMBAT) && (ListUnits[y].position.DistanceTo(pos) < 15.0f)
			&& !Functions::Intersect(ListUnits[y].position, pos, 2.00f)) {
			return true;
		}
	}
	return false;
}

bool MoveObstacle_tmp(Position target_pos, Position last_pos) {
	float angle_targetPos = atan2f(target_pos.Y - last_pos.Y, target_pos.X - last_pos.X);
	if (angle_targetPos < 0.0f) angle_targetPos += halfPI * 4.0f;
	else if (angle_targetPos > halfPI * 4) angle_targetPos -= halfPI * 4.0f;
	Position tmp_pos, next_pos = last_pos;
	while (next_pos.DistanceTo(target_pos) > 2.5f) {
		tmp_pos = Position((cos(angle_targetPos) * 2.5f) + last_pos.X
			, (sin(angle_targetPos) * 2.5f) + last_pos.Y, last_pos.Z);
		next_pos = Functions::ProjectPos(tmp_pos, 2.00f);
		if (Functions::Intersect(last_pos, next_pos, 2.00f) || (Functions::GetDepth(tmp_pos, 2.00f) > 2.00f)) return false;
		last_pos = next_pos;
	}
	return true;
}

void Functions::MoveObstacle(Position target_pos) {
	float angle_targetPos = atan2f(target_pos.Y - localPlayer->position.Y, target_pos.X - localPlayer->position.X);
	if (angle_targetPos < 0.0f) angle_targetPos += halfPI * 4.0f;
	else if (angle_targetPos > halfPI * 4) angle_targetPos -= halfPI * 4.0f;
	int j = 0;
	for (int i = 0; i < 6; i++) { //Front, left, right...
		for (int z = 0; z < 2; z++) {
			if (z == 0) j = 16 - i;
			else {
				if (i == 0) break;
				j = i;
			}
			Position last_pos = localPlayer->position; //Take into account the difference in altitude at each point
			for (int w = 0; w < 12; w++) { //Every 2.5 yards up to 30 check for LoS point
				Position tmp_pos = Position((cos(angle_targetPos + (j * halfPI / 4)) * 2.5f) + last_pos.X
					, (sin(angle_targetPos + (j * halfPI / 4)) * 2.5f) + last_pos.Y, last_pos.Z);
				Position next_pos = Functions::ProjectPos(tmp_pos, 2.00f);
				if (!Functions::Intersect(last_pos, next_pos, 2.00f) && (Functions::GetDepth(tmp_pos, 2.00f) < 2.00f)) {
					if (Functions::EnemyClose(next_pos)) break; //Change direction
					else if (MoveObstacle_tmp(target_pos, next_pos)) {
						if(i > 0) localPlayer->ClickToMove(Move, localPlayer->Guid, next_pos);
						else localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
						return;
					}
					else { last_pos = next_pos; continue; }
				}
				else break; //There is an obstacle on this path, we need to change
			}
		}
	}
}

void Functions::MoveLoS(Position target_pos) {
	/*
		Check every directions for a position where you have line of sight of target_pos,
		if there is an obstacle on the path check a new direction
	*/
	float angle_targetPos = atan2f(target_pos.Y - localPlayer->position.Y, target_pos.X - localPlayer->position.X);
	if (angle_targetPos < 0.0f) angle_targetPos += halfPI * 4.0f;
	else if (angle_targetPos > halfPI * 4) angle_targetPos -= halfPI * 4.0f;
	int j = 0;
	for (int i = 1; i < 6; i++) { //Left, right...
		for (int z = 0; z < 2; z++) {
			Position last_pos = localPlayer->position; //Take into account the difference in altitude at each point
			if (z == 0) j = 16 - i; else j = i; //Pendulum direction choice
			for (int w = 0; w < 12; w++) { //Every 2.5 yards up to 30 check for LoS point
				Position tmp_pos = Position((cos(angle_targetPos + (j * halfPI / 4)) * 2.5f) + last_pos.X
					, (sin(angle_targetPos + (j * halfPI / 4)) * 2.5f) + last_pos.Y, last_pos.Z);
				Position next_pos = Functions::ProjectPos(tmp_pos, 2.00f);
				if (!Functions::Intersect(last_pos, next_pos, 2.00f) && (Functions::GetDepth(tmp_pos, 2.00f) < 2.00f)) {
					if (Functions::EnemyClose(next_pos)) break; //Change direction
					else if (!Functions::Intersect(next_pos, target_pos, 2.00f)) {
						localPlayer->ClickToMove(Move, localPlayer->Guid, next_pos);
						Moving = 6;
						return;
					}
					else { last_pos = next_pos; continue; }
				}
				else break; //There is an obstacle on this path, we need to change
			}
		}
	}
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

std::tuple<Position, int> Functions::getAOETargetPos(float range, float range2) {
	//Execution: ~0.05ms
	//Hierarchical Clustering implementation to find position
	std::vector<std::vector<Position>> clustersArr;
	std::vector<Position> clusters_center;
	//1- Chaque position est un cluster
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].attackable) {
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
						if (distCluster < distMin && distCluster < (range / 2)) {
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
		//3- On choisit le cluster avec le plus d'unit�s
		unsigned int max_cluster_unit = 0; int index = 0;
		for (unsigned int i = 0; i < clustersArr.size(); i++) {
			if (clustersArr[i].size() > max_cluster_unit && clusters_center[i].DistanceTo(localPlayer->position) < range2) {
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
	ccTarget = NULL;
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if (ListUnits[i].attackable) { //Hostile
			bool aggroed = false;
			for (int y = 0; y <= NumGroupMembers; y++) {
				if ((GroupMember[y] != NULL) && (GroupMember[y]->Guid == ListUnits[i].targetGuid) && (ListUnits[i].flags & UNIT_FLAG_IN_COMBAT)) {
					HasAggro[y].push_back(&ListUnits[i]); //Check if Group Member has aggro
					if(y == 0) aggroed = true;
					break;
				}
			}
			bool groupMember = false;
			if (ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED) { //Enemy player
				for (int y = 0; y <= NumGroupMembers; y++) {
					if ((GroupMember[y] != NULL) && (GroupMember[y]->Guid == ListUnits[i].Guid)) {
						groupMember = true;
						break;
					}
				}
				if(!groupMember) nbrEnemyPlayer++;
				if((targetUnit != NULL) && (ListUnits[i].Guid != targetUnit->Guid)) {
					float dist = localPlayer->position.DistanceTo(ListUnits[i].position);
					if (dist < 30) ccTarget = &ListUnits[i]; //2nd Target for CC
				}
			}
			if (!groupMember && (ListUnits[i].flags & UNIT_FLAG_IN_COMBAT)) { //Combat
				if(!(ListUnits[i].flags & UNIT_FLAG_CONFUSED)) nbr++;
				if (localPlayer->position.DistanceTo(ListUnits[i].position) < 7.5f) {
					if (!(ListUnits[i].flags & UNIT_FLAG_CONFUSED)) nbrClose++;
					if (localPlayer->isFacing(ListUnits[i].position, 0.5f)) {
						nbrCloseFacing++;
					}
				}
			}
		}
	}
	return std::make_tuple(nbr, nbrClose, nbrCloseFacing, nbrEnemyPlayer);
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

WoWUnit* Functions::GetMissingBuff(int* IDs, int size) {
	//Retourne le joueur auquel il manque le buff
	for (int i = 1; i <= NumGroupMembers; i++) {
		if ((GroupMember[i] != NULL) && (GroupMember[i]->unitReaction > Neutral)
			&& !GroupMember[i]->isdead && (localPlayer->position.DistanceTo(GroupMember[i]->position) < 40.0f)
			&& !Intersect(localPlayer->position, GroupMember[i]->position, 2.00f) && !GroupMember[i]->hasBuff(IDs, size)) return GroupMember[i];
	}
	return NULL;
}

unsigned int Functions::GetMapID() {
	typedef int __fastcall func();
	func* function = (func*)GETMAPID_FUN_PTR;
	return function();
}

bool Functions::PlayerIsRanged() {
	if(playerClass == "Mage" || playerClass == "Priest" || playerClass == "Warlock" || playerClass == "Hunter"
		|| (playerClass == "Druid" && (playerSpec == 0 || playerSpec == 2)) || (playerClass == "Shaman" && (playerSpec == 0 || playerSpec == 2))) return true;
	else return false;
}

//======================================================================//
//========================   Game Functions   ==========================//
//======================================================================//

int GetIntFromChar(const char* txt) {
	//Obtient le !premier! nombre d'un char*
	int length = strlen(txt); bool neg = false;
	int ret = 0; int y = 0; int lenNbr = 0;
	for (int i = 0; i < length; i++) {
		if (lenNbr == 0 && txt[i] == '-') neg = true;
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') lenNbr++;
		else if (lenNbr > 0) break;
	}
	for (int i = 0; i < length; i++) {
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') {
			ret = ret + (pow(10, lenNbr - y - 1) * ((int)txt[i] - '0'));
			y++;
		}
		else if (y > 0) break;
	}
	if (neg) return -ret;
	else return ret;
}

float GetFloatFromChar(const char* txt) {
	int length = strlen(txt); int y = 0;
	float ret = 0; int indDecimal = 0;
	for (int i = 0; i < length; i++) {
		if (txt[i] == ',' || txt[i] == '.') indDecimal = i;
	}
	for (int i = 0; i < length; i++) {
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') {
			if(indDecimal > 0)
				ret = ret + (pow(10, length - (length - indDecimal) - y - 1) * (float)((int)txt[i] - '0'));
			else
				ret = ret + (pow(10, length - y - 1) * (float)((int)txt[i] - '0'));
			y++;
		}
	}
	return ret;
}

int Functions::GetNumGroupMembers() {
	LuaCall("count = GetNumRaidMembers()");
	int nbrRaid = GetIntFromChar((char*)GetText("count"));
	LuaCall("count = GetNumPartyMembers()");
	int nbrParty = GetIntFromChar((char*)GetText("count"));
	if (nbrRaid > nbrParty) return nbrRaid;
	else return nbrParty;
}

bool Functions::IsInGroup() {
	LuaCall("count = GetNumPartyMembers()");
	int nbrParty = GetIntFromChar((char*)GetText("count"));
	if (nbrParty > 0) return true;
	else return false;
}

bool Functions::IsInRaid() {
	LuaCall("count = GetNumRaidMembers()");
	int nbrRaid = GetIntFromChar((char*)GetText("count"));
	if (nbrRaid > 0) return true;
	else return false;
}

int Functions::GetMerchantNumItems() {
	LuaCall("count = GetMerchantNumItems()");
	int nbr = GetIntFromChar((char*)GetText("count"));
	return nbr;
}

int Functions::GetRepairAllCost() {
	LuaCall("cost = GetRepairAllCost()");
	int cost = GetIntFromChar((char*)GetText("cost"));
	return cost;
}

bool Functions::HasPetUI() {
	LuaCall("UI = HasPetUI()");
	int hasUI = GetIntFromChar((char*)GetText("UI"));
	if (hasUI == 1) return true;
	else return false;
}

int Functions::GetPetHappiness() {
	LuaCall("hap = GetPetHappiness()");
	int hapiness = GetIntFromChar((char*)GetText("hap"));
	return hapiness;
}

void Functions::SellUselessItems() {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			if (GetItemQuality(i, y) == 0) {
				std::string command = "UseContainerItem(" + std::to_string(i) + ", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
				return;
			}
		}
	}
}

//======================================================================//
//============================   Timer/CD   ============================//
//======================================================================//

float Functions::GetTime() {
	LuaCall("time = GetTime()");
	float time = GetFloatFromChar((char*)GetText("time"));
	return time;
}

float Functions::GetItemCooldownDuration(int item_id) {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			if (link_nbr == item_id) {
				std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(i) + +", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
				float start = GetFloatFromChar((char*)GetText("start"));
				float duration = GetFloatFromChar((char*)GetText("duration"));
				float cdLeft = start + duration - GetTime();
				if (cdLeft < 0) cdLeft = 0;
				return cdLeft;
			}
		}
	}
	return 999;
}

float Functions::GetItemCooldownDuration(int* items_id, int size) {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			for (int z = 0; z < size; z++) {
				if (link_nbr == items_id[z]) {
					std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(i) + +", " + std::to_string(y) + ")";
					LuaCall(command.c_str());
					float start = GetFloatFromChar((char*)GetText("start"));
					float duration = GetFloatFromChar((char*)GetText("duration"));
					float cdLeft = start + duration - GetTime();
					if (cdLeft < 0) cdLeft = 0;
					return cdLeft;
				}
			}
		}
	}
	return 999;
}

float Functions::GetActionCooldownDuration(int slot) {
	std::string command = "start, duration = GetActionCooldown(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	float start = GetFloatFromChar((char*)GetText("start"));
	float duration = GetFloatFromChar((char*)GetText("duration"));
	float cdLeft = start + duration - GetTime();
	if (cdLeft < 0) cdLeft = 0;
	return cdLeft;
}

float Functions::GetSpellCooldownDuration(std::string spell_name) {
	int spell_id;
	std::tie(spell_id, std::ignore) = GetSpellID(spell_name);
	if (spell_id > 0) {
		std::string command = "start, duration = GetSpellCooldown(" + std::to_string(spell_id) + ", BOOKTYPE_SPELL)";
		LuaCall(command.c_str());
		float start = GetFloatFromChar((char*)GetText("start"));
		float duration = GetFloatFromChar((char*)GetText("duration"));
		float cdLeft = start + duration - GetTime();
		if (cdLeft < 0) cdLeft = 0;
		return cdLeft;
	} else return 999;
}

//======================================================================//
//=============================   Items   ==============================//
//======================================================================//

int Functions::GetContainerNumSlots(int slot) {
	std::string command = "slots = GetContainerNumSlots(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int slots = GetIntFromChar((char*)GetText("slots"));
	return slots;
}

std::tuple<std::string, int> Functions::GetContainerItemInfo(int bag, int slot) {
	std::string command = "texture, itemCount = GetContainerItemInfo("+std::to_string(bag)+", "+std::to_string(slot)+")";
	LuaCall(command.c_str());
	std::string texture = (char*)GetText("texture");
	int itemCount = GetIntFromChar((char*)GetText("itemCount"));
	return std::make_tuple(texture, itemCount);
}

std::string Functions::GetContainerItemLink(int bag, int slot) {
	std::string command = "link = GetContainerItemLink(" + std::to_string(bag) + ", " + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	std::string link = (char*)GetText("link");
	return link;
}

bool Functions::IsInventoryFull() {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string texture;
			std::tie(texture, std::ignore) = GetContainerItemInfo(i, y);
			if (texture == "") return false;
		}
	}
	return true;
}

int Functions::GetItemCount(std::string item_info) {
	//Trouve par le nom la quantit� d'item similaire dans l'inventaire
	int total = 0;
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			if (item_link.find(item_info) != std::string::npos) {
				int itemCount;
				std::tie(std::ignore, itemCount) = GetContainerItemInfo(i, y);
				total = total + itemCount;
			}
		}
	}
	return total;
}

int Functions::GetItemCount(int item_id) {
	//Trouve par l'ID la quantit� d'item similaire dans l'inventaire
	int total = 0;
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			if (link_nbr == item_id) {
				int itemCount;
				std::tie(std::ignore, itemCount) = GetContainerItemInfo(i, y);
				total = total + itemCount;
			}
		}
	}
	return total;
}

int Functions::HasItem(int* item_ids, int size) {
	//Trouve par l'ID si un des items de la liste est pr�sent
	int total = 0;
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			for (int z = 0; z < size; z++) {
				if (link_nbr == item_ids[z]) {
					int itemCount;
					std::tie(std::ignore, itemCount) = GetContainerItemInfo(i, y);
					return item_ids[z];
				}
			}
		}
	}
	return 0;
}

int Functions::GetItemQuality(int bag, int slot) {
	std::string item_link = GetContainerItemLink(bag, slot);
	if (item_link == "") return -1;
	else if (item_link.find("9d9d9d") != std::string::npos) return 0;	//Poor
	else if (item_link.find("ffffff") != std::string::npos) return 1;	//Common
	else if (item_link.find("1eff00") != std::string::npos) return 2;	//Uncommon
	else if (item_link.find("0070dd") != std::string::npos) return 3;	//Rare
	else if (item_link.find("a335ee") != std::string::npos) return 4;	//Epic
	else if (item_link.find("ff8000") != std::string::npos) return 5;	//Legendary
	else if (item_link.find("e6cc80") != std::string::npos) return 6;	//Artifact
	else return -1;
}

void Functions::PickupItem(std::string item_info) {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			if (item_link.find(item_info) != std::string::npos) {
				std::string command = "PickupContainerItem(" + std::to_string(i) + ", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
			}
		}
	}
}

void Functions::PickupItem(int item_id) {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			if (link_nbr == item_id) {
				std::string command = "PickupContainerItem(" + std::to_string(i) + ", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
			}
		}
	}
}

void Functions::PlaceItem(int slot, std::string itemName) {
	//Place l'objet dans le slot indiqu�
	PickupItem(itemName);
	std::string command = "PlaceAction(" + std::to_string(slot) + ")";
	LuaCall((command + " ClearCursor()").c_str());
}

void Functions::PlaceItem(int slot, int item_id) {
	//Place l'objet dans le slot indiqu�
	PickupItem(item_id);
	std::string command = "PlaceAction(" + std::to_string(slot) + ")";
	LuaCall((command + " ClearCursor()").c_str());
}

void Functions::UseItem(std::string item_info) {
	//Use the indicated item
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			if (item_link.find(item_info) != std::string::npos) {
				std::string command = "UseContainerItem(" + std::to_string(i) + ", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
			}
		}
	}
}

void Functions::UseItem(int item_id) {
	//Use the indicated item
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			int link_nbr = GetIntFromChar(item_link.c_str());
			if (link_nbr == item_id) {
				std::string command = "UseContainerItem(" + std::to_string(i) + ", " + std::to_string(y) + ")";
				LuaCall(command.c_str());
				return;
			}
		}
	}
}

int Functions::HasDrink() {
	int listID[20] = { 159, 1179, 1205, 1645, 1708, 2136, 2288, 3772, 4791, 5350, 8077
		, 8078, 8079, 8766, 9451, 10841, 13724, 18300, 19301, 20031 };
	return HasItem(listID, 20);
}

int Functions::HasMeat() {
	int CookedlistID[47] = { 117, 724, 1017, 2287, 2679, 2680, 2681, 2684, 2685, 2687, 2888, 3220, 3662, 3664, 3726, 3727, 3728, 3770, 3771, 4457, 4599, 5472, 5474, 5477, 5478, 5479, 7097, 8952, 11444, 12209, 12210, 12211, 12213, 12215, 12216, 12224, 13851, 17119, 17222, 17407, 17408, 18045, 19224, 19304, 19305, 20074, 21023 };
	return HasItem(CookedlistID, 47);
}

bool Functions::HasHPotion() {
	int listID[6] = { 118, 858, 929, 1710, 3928, 13446};
	if (HasItem(listID, 6) > 0) return true;
	else return false;
}

float Functions::GetHPotionCD() {
	int listID[6] = { 118, 858, 929, 1710, 3928, 13446 };
	float CD = GetItemCooldownDuration(listID, 6);
	return CD;
}

bool Functions::HasMPotion() {
	int listID[6] = { 2455, 3385, 3827, 6149, 13443, 13444 };
	if (HasItem(listID, 6) > 0) return true;
	else return false;
}

float Functions::GetMPotionCD() {
	int listID[6] = { 2455, 3385, 3827, 6149, 13443, 13444 };
	float CD = GetItemCooldownDuration(listID, 6);
	return CD;
}

bool Functions::HasHealthstone() {
	int listID[] = { 5512, 19004, 19005, 5511, 19006, 19007, 5509, 19008, 19009, 5510, 19010, 19011, 9421, 19012, 19013 };
	if (HasItem(listID, 15) > 0) return true;
	else return false;
}

float Functions::GetHealthstoneCD() {
	int listID[15] = { 5512, 19004, 19005, 5511, 19006, 19007, 5509, 19008, 19009, 5510, 19010, 19011, 9421, 19012, 19013 };
	float CD = GetItemCooldownDuration(listID, 15);
	return CD;
}

//======================================================================//
//========================   Buffs / Debuffs   =========================//
//======================================================================//

std::string Functions::UnitBuff(std::string target, int index) {
	std::string command = "texture = UnitBuff(\"" + (std::string)target + "\", " + std::to_string(index) + ")";
	LuaCall(command.c_str());
	char* texture = (char*)GetText("texture");
	return texture;
}

std::tuple<std::string, int, std::string> Functions::UnitDebuff(std::string target, int index) {
	std::string command = "texture,count,type = UnitDebuff(\"" + (std::string)target + "\", " + std::to_string(index) + ")";
	LuaCall(command.c_str());
	char* texture = (char*)GetText("texture");
	int count = GetIntFromChar((char*)GetText("count"));
	char* type = (char*)GetText("type");
	return std::make_tuple(texture, count, type);
}

bool Functions::GetUnitBuff(std::string target, std::string texture) {
	for (int i = 1; i <= 30; i++) {
		std::string textname = UnitBuff(target, i);
		if (textname == texture) return true;
		else if (textname == "") return false;
	}
	return false;
}

bool Functions::GetUnitDebuff(std::string target, std::string texture) {
	for (int i = 1; i <= 16; i++) {
		std::string textname;
		std::tie(textname, std::ignore, std::ignore) = UnitDebuff(target, i);
		if (textname == texture) return true;
		else if (textname == "") return false;
	}
	return false;
}

int Functions::GetStackDebuff(std::string target, std::string texture) {
	for (int i = 1; i <= 16; i++) {
		int count;
		std::tie(std::ignore, count, std::ignore) = UnitDebuff(target, i);
		return count;
	}
	return 0;
}

bool Functions::GetUnitDispel(std::string target, std::string dispellType1, std::string dispellType2, std::string dispellType3) {
	//Retourne si la cible a un debuff � dispel
	std::string args[3] = { dispellType1, dispellType2, dispellType3 };
	for (int i = 1; i <= 16; i++) {
		std::string debuffIcon, debuffType;
		std::tie(debuffIcon, std::ignore, debuffType) = UnitDebuff(target, i);
		if (debuffIcon != "Interface\\Icons\\Spell_Frost_FrostArmor02") {
			for (int y = 0; y < 3; y++) {
				if (args[y] == debuffType) return true;
			}
		}
	}
	return false;
}

WoWUnit* Functions::GetGroupDispel(std::string dispellType1, std::string dispellType2, std::string dispellType3) {
	//Retourne le joueur du groupe � dispel
	for (int i = 1; i <= NumGroupMembers; i++) {
		if ((GroupMember[i] != NULL) && (GroupMember[i]->unitReaction > Neutral)
			&& !GroupMember[i]->isdead && (localPlayer->position.DistanceTo(GroupMember[i]->position) < 40.0f)
			&& !Intersect(localPlayer->position, GroupMember[i]->position, 2.00f)
			&& GetUnitDispel(tarType + std::to_string(i), dispellType1, dispellType2, dispellType3)) return GroupMember[i];
	}
	return NULL;
}

//======================================================================//
//=============================   Status   =============================//
//======================================================================//

bool Functions::IsSlowed(std::string target) {
	//Return if target is rooted
	std::vector<std::string> tab;
	tab.push_back("Interface\\Icons\\Spell_Frost_Glacier");
	tab.push_back("Interface\\Icons\\Ability_Rogue_Trip");
	for (int i = 1; i <= 16; i++) {
		std::string debuff;
		std::tie(debuff, std::ignore, std::ignore) = UnitDebuff(target, i);
		for (unsigned int y = 0; y < tab.size(); y++) {
			if (debuff == tab[y]) return true;
		}
	}
	return false;
}

bool Functions::IsRooted(std::string target) {
	//Return if target is rooted
	std::vector<std::string> tab;
	tab.push_back("Interface\\Icons\\Spell_Frost_FrostNova");
	tab.push_back("Interface\\Icons\\Spell_Frost_FrostArmor");
	tab.push_back("Interface\\Icons\\Spell_Nature_StrangleVines");
	tab.push_back("Interface\\Icons\\Ability_Ensnare");
	for (int i = 1; i <= 16; i++) {
		std::string debuff;
		std::tie(debuff, std::ignore, std::ignore) = UnitDebuff(target, i);
		for (unsigned int y = 0; y < tab.size(); y++) {
			if (debuff == tab[y]) return true;
		}
	}
	return false;
}

bool Functions::IsFeared(std::string target) {
	//Return if target is feared
	std::vector<std::string> tab;
	tab.push_back("Interface\\Icons\\Spell_Shadow_Possession");
	tab.push_back("Interface\\Icons\\Spell_Shadow_PsychicScream");
	tab.push_back("Interface\\Icons\\Spell_Shadow_DeathCoil");
	tab.push_back("Interface\\Icons\\Ability_GolemThunderClap");
	tab.push_back("Interface\\Icons\\Ability_Physical_Taunt");
	for (int i = 1; i <= 16; i++) {
		std::string debuff;
		std::tie(debuff, std::ignore, std::ignore) = UnitDebuff(target, i);
		for (unsigned int y = 0; y < tab.size(); y++) {
			if (debuff == tab[y]) return true;
		}
	}
	return false;
}

bool Functions::IsCharmed(std::string target) {
	//Return if target is charmed
	LuaCall("zoneText = GetRealZoneText()");
	std::string GetRealZoneText = (char*)GetText("zoneText");
	std::vector<std::pair<std::string, std::string>> tab;
	tab.push_back(std::make_pair("Interface\\Icons\\Spell_Shadow_ShadowWordDominate", ""));
	tab.push_back(std::make_pair("Interface\\Icons\\Spell_Shadow_MindSteal", ""));
	tab.push_back(std::make_pair("Interface\\Icons\\Spell_Shadow_GatherShadows", "Shadowfang Keep"));
	for (int i = 1; i <= 16; i++) {
		std::string debuff;
		std::tie(debuff, std::ignore, std::ignore) = UnitDebuff(target, i);
		for (unsigned int y = 0; y < tab.size(); y++) {
			if (debuff == tab[y].first && (tab[y].second == "" || tab[y].second == GetRealZoneText)) return true;
		}
	}
	return false;
}

//======================================================================//
//=========================   Spells/Actions   =========================//
//======================================================================//

bool Functions::GetShapeshiftFormInfo(int nbr) {
	LuaCall(("_,_,Stance = GetShapeshiftFormInfo(" + std::to_string(nbr) + ")").c_str());
	int stance = GetIntFromChar((char*)GetText("Stance"));
	if (stance == 1) return true;
	else return false;
}

int Functions::GetNumSpellTabs() {
	LuaCall("res = GetNumSpellTabs()");
	int result = GetIntFromChar((char*)GetText("res"));
	return result;
}

std::string Functions::GetSpellName(int id) {
	std::string command = "res = GetSpellName(" + std::to_string(id) + ", BOOKTYPE_SPELL)";
	LuaCall(command.c_str());
	char* result = (char*)GetText("res");
	return result;
}

std::string Functions::GetSpellTexture(int spellID) {
	std::string command = "res = GetSpellTexture(" + std::to_string(spellID) + ", BOOKTYPE_SPELL)";
	LuaCall(command.c_str());
	std::string result = (char*)GetText("res");
	return result;
}

std::tuple<std::string, std::string, int, int> Functions::GetSpellTabInfo(int index) {
	std::string command = "name, texture, offset, numSpells = GetSpellTabInfo(" + std::to_string(index) + ")";
	LuaCall(command.c_str());
	char* name = (char*)GetText("name");
	char* texture = (char*)GetText("texture");
	int offset = GetIntFromChar((char*)GetText("offset"));
	int numSpells = GetIntFromChar((char*)GetText("numSpells"));
	return std::make_tuple(name, texture, offset, numSpells);
}

std::tuple<int, int> Functions::GetSpellID(std::string spell_name) {
	//Execution = 1ms
	int id = 0; int rank = 0;
	for (int i = 1; i <= GetNumSpellTabs(); i++) {
		int numSpells;
		std::tie(std::ignore, std::ignore, std::ignore, numSpells) = GetSpellTabInfo(i);
		for (int y = 0; y < numSpells; y++) {
			id++;
			if (spell_name == GetSpellName(id)) {
				while (spell_name == GetSpellName(id + 1)) {
					id++; rank++;
				}
				return std::make_tuple(id, rank);
			}
		}
	}
	return std::make_tuple(0, 0);
}

bool Functions::IsPlayerSpell(std::string spell_name) {
	int spellID;
	std::tie(spellID, std::ignore) = GetSpellID(spell_name);
	if (spellID == 0) return false;
	else return true;
}

bool Functions::IsSpellReady(std::string spell_name) {
	//Execution: ~2.2ms
	if (((localPlayer->flags & UNIT_FLAG_SILENCED) != UNIT_FLAG_SILENCED)) {
		int slot = GetSlot(spell_name);
		if (slot > 0) {
			if (IsUsableAction(slot) && (GetActionCooldownDuration(slot) < 1.25f)) {
				return true;
			}
		}
	}
	return false;
}

void Functions::CastSpellByName(std::string spell_name) {
	LuaCall(("CastSpellByName(\"" + spell_name + "\")").c_str());
}

void Functions::UseAction(int slot, int self) {
	if(self == 1) LuaCall(("UseAction(" + std::to_string(slot) + ", 0, 1)").c_str());
	else LuaCall(("UseAction("+std::to_string(slot)+")").c_str());
}

bool Functions::IsAutoRepeatAction(int slot) {
	LuaCall(("autoR = IsAutoRepeatAction(" + std::to_string(slot) + ")").c_str());
	int autoR = GetIntFromChar((char*)GetText("autoR"));
	if (autoR == 1) return true;
	else return false;
}

bool Functions::IsUsableAction(int slot) {
	std::string command = "usable, nomana = IsUsableAction(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int usable = GetIntFromChar((char*)GetText("usable"));
	int nomana = GetIntFromChar((char*)GetText("nomana"));
	if (usable == 1 && nomana == 0) return true;
	else return false;
}

bool Functions::HasAction(int slot) {
	std::string command = "res = HasAction(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("res"));
	if (result == 1) return true;
	else return false;
}

std::string Functions::GetActionTexture(int slot) {
	std::string command = "res = GetActionTexture(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	std::string result = (char*)GetText("res");
	return result;
}

bool Functions::IsConsumableAction(int slot) {
	std::string command = "res = IsConsumableAction(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("res"));
	if (result == 1) return true;
	else return false;
}

bool Functions::IsActionInRange(int slot) {
	std::string command = "res = IsActionInRange(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("res"));
	if (result == 1) return true;
	else return false;
}

int Functions::GetSlot(std::string spell_name, std::string slot_type) {
	//Execution: 2ms
	int slot = 0; int spellID;
	std::tie(spellID, std::ignore) = GetSpellID(spell_name);
	if (spellID > 0) {
		for (int i = 1; i < 120; i++) {
			if (HasAction(i) && (GetSpellTexture(spellID) == GetActionTexture(i))
				&& ((slot_type == "SPELL" && !IsConsumableAction(i))
				|| (slot_type == "ITEM" && IsConsumableAction(i)))) {
				slot = i;
			}
		}
	}
	return slot;
}

//======================================================================//
//=============================   Units   ==============================//
//======================================================================//

WoWUnit* Functions::GetHealer() {
	for (int i = 1; i <= NumGroupMembers; i++) {
		if (GroupMember[i] == NULL) continue;
		std::string grClass = UnitClass(tarType+std::to_string(i));
		if (grClass == "Priest" || grClass == "Paladin" || grClass == "Shaman") return GroupMember[i];
	}
	return NULL;
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

int Functions::UnitStat(std::string target, int nbr) {
	LuaCall(("stat = UnitStat(\"" + target + "\", " + std::to_string(nbr) + ")").c_str());
	int stat = GetIntFromChar((char*)GetText("stat"));
	return stat;
}

void Functions::TargetUnit(std::string target) {
	LuaCall(("TargetUnit(\"" + target + "\")").c_str());
}

std::string Functions::UnitName(std::string target) {
	LuaCall(("name = UnitName(\"" + target + "\")").c_str());
	std::string name = (char*)GetText("name");
	return name;
}

bool Functions::UnitCanAttack(std::string char1, std::string char2) {
	std::string command = "canAttack = UnitCanAttack(\"" + char1 + "\", \"" + char2 + "\")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("canAttack"));
	if (result == 1) return true;
	else return false;
}

bool Functions::UnitIsDeadOrGhost(std::string char1) {
	std::string command = "dead = UnitIsDeadOrGhost(\"" + char1 + "\")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("dead"));
	if (result == 1) return true;
	else return false;
}

bool Functions::CheckInteractDistance(std::string char1, int dist) {
	std::string command = "checkInteract = CheckInteractDistance(\"" + char1 + "\", " + std::to_string(dist) + ")";
	LuaCall(command.c_str());
	int result = GetIntFromChar((char*)GetText("checkInteract"));
	if (result == 1) return true;
	else return false;
}

bool Functions::UnitAffectingCombat(std::string target) {
	std::string command = "cb = UnitAffectingCombat(\"" + target + "\")";
	LuaCall(command.c_str());
	int cb = GetIntFromChar((char*)GetText("cb"));
	if (cb == 1) return true;
	else return false;
}

std::string Functions::UnitClass(std::string target) {
	LuaCall(("class = UnitClass(\"" + target + "\")").c_str());
	std::string tarClass = (char*)GetText("class");
	return tarClass;
}

bool Functions::UnitIsCaster(std::string target) {
	std::string tarClass = UnitClass(target);
	if (tarClass == "Priest" || tarClass == "Warlock" || tarClass == "Mage") return true;
	else if (target == "player" && ((tarClass == "Druid" && (playerSpec == 0 || playerSpec == 2)) || (tarClass == "Shaman" && (playerSpec == 0 || playerSpec == 2)))) return true;
	else return false;
}

bool Functions::UnitIsElite(std::string target) {
	LuaCall(("classification = UnitClassification(\"" + target + "\")").c_str());
	std::string classification = (char*)GetText("classification");
	LuaCall(("level = UnitLevel(\"" + target + "\")").c_str());
	int level = GetIntFromChar((char*)GetText("level"));
	LuaCall(("playerCtrl = UnitPlayerControlled(\"" + target + "\")").c_str());
	int playerCtrl = GetIntFromChar((char*)GetText("playerCtrl"));
	if (classification == "elite" || classification == "rareelite" || level == -1 || playerCtrl == 1) return true;
	else return false;
}

std::string Functions::UnitCreatureType(std::string target) {
	LuaCall(("type = UnitCreatureType(\"" + target + "\")").c_str());
	std::string type = (char*)GetText("type");
	return type;
}

bool Functions::IsGroupInCombat() {
	for (int i = 1; i <= NumGroupMembers; i++) {
		if(UnitAffectingCombat(tarType+std::to_string(i))) return true;
	}
	return false;
}

bool Functions::IsShieldEquipped() {
	LuaCall("_, _, id = string.find(GetInventoryItemLink(\"player\", GetInventorySlotInfo(\"SecondaryHandSlot\")) or \"\", \"(item:%d+:%d+:%d+:%d+)\")");
	int id = GetIntFromChar((char*)GetText("id"));
	if (id > 0) {
		std::string command = "_, _, _, _, itemType = GetItemInfo(" + std::to_string(id) + ")";
		LuaCall(command.c_str());
		std::string itemType = (char*)GetText("itemType");
		if (itemType == "Armor") return true;
	}
	return false;
}

bool Functions::HasWandEquipped() {
	LuaCall("wand = HasWandEquipped()");
	int wand = GetIntFromChar((char*)GetText("wand"));
	if (wand == 1) return true;
	else return false;
}

int Functions::GetComboPoints() {
	LuaCall("pts = GetComboPoints()");
	int pts = GetIntFromChar((char*)GetText("pts"));
	return pts;
}

int Functions::GetTalentInfo(int page, int index) {
	std::string command = "_,_,_,_,pts = GetTalentInfo(" + std::to_string(page) + ", " + std::to_string(index) + ")";
	LuaCall(command.c_str());
	int pts = GetIntFromChar((char*)GetText("pts"));
	return pts;
}

bool Functions::IsCurrentAction(int slot) {
	std::string command = "current = IsCurrentAction(" + std::to_string(slot) + ")";
	LuaCall(command.c_str());
	int current = GetIntFromChar((char*)GetText("current"));
	if (current == 1) return true;
	else return false;
}

float Functions::UnitAttackSpeed(std::string target) {
	std::string command = "aaspeed = UnitAttackSpeed(\"" + target + "\")";
	LuaCall(command.c_str());
	float aaspeed = GetFloatFromChar((char*)GetText("aaspeed"));
	return aaspeed;
}

void Functions::FollowUnit(std::string target) {
	std::string command = "FollowUnit(\"" + target + "\")";
	LuaCall(command.c_str());
}