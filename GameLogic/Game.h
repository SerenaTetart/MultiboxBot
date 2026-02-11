#pragma once
#include <vector>
#include <string>

#include "WoWObject.h"


class Game {
public:
	static void MainLoop();
private:
	static void CorpseRun();
	static void TrainSpellRun();
};

//Global Variables
extern bool Combat, IsSitting, IsFacing, hasTargetAggro, MCNoAuto, MCAutoMove, los_target, passiveGroup;
extern float distTarget;
extern std::vector<WoWUnit*> HasAggro[40];
extern std::vector<std::tuple<unsigned long long, time_t>> LootHistory;
extern std::vector<int> HealTargetArray;
extern int AoEHeal, nbrEnemy, nbrCloseEnemy, nbrCloseEnemyFacing, nbrEnemyPlayer, Moving, NumGroupMembers, playerSpec, positionCircle,
			skinningLevel, miningLevel, herbalismLevel, mapID, keybindTrigger, IsInGroup, autoLearnSpells;
extern unsigned int LastTarget;
extern std::string tarType;
extern std::vector<std::tuple<std::string, int, int, int>> leaderInfos; // Nom, role, trade skill1, trade skill2
extern std::vector<std::tuple<int, int, int, std::string>> virtualInventory;
extern WoWUnit* ccTarget; extern WoWUnit* targetUnit; extern WoWUnit* GroupMember[40]; extern WoWUnit* PartyMember[5]; extern WoWUnit* Leader;
extern time_t current_time, gatheringCD;
extern Position playerLastPos;
