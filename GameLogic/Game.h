#pragma once
#include <vector>
#include <string>

#include "WoWObject.h"
#include "./data/Inventory.h"
#include "./data/Spell.h"

class Game {
public:
	static void MainLoop();
	static void UseMount();
	static void Dismount();
	static bool Loot();
	static bool Trade();
	static bool Disenchant();
private:
	static void CorpseRun();
	static void DoChores();
};

//Global Variables
extern bool Combat, IsSitting, IsFacing, hasTargetAggro, MCNoAuto, MCAutoMove, los_target, passiveGroup, inInstance, inventoryFull;
extern float distTarget, autoAttackTimer, breathTimer;
extern std::vector<WoWUnit*> HasAggro[40];
extern std::vector<std::tuple<unsigned long long, time_t>> LootHistory;
extern std::vector<int> HealTargetArray;
extern int AoEHeal, nbrEnemy, nbrCloseEnemy, nbrCloseEnemyFacing, nbrEnemyPlayer, Moving, NumGroupMembers, playerSpec, positionCircle,
			skinningLevel, miningLevel, herbalismLevel, mapID, keybindTrigger, IsInGroup, autoChores;
extern unsigned int LastTarget;
extern std::string tarType;
extern std::vector<std::tuple<std::string, int, int, int>> leaderInfos; // Nom, role, trade skill1, trade skill2
extern std::vector<InventoryItem> virtualInventory;
extern std::vector<SpellSlotData> virtualSpellBook;
extern WoWUnit* ccTarget; extern WoWUnit* targetUnit; extern WoWUnit* GroupMember[40]; extern WoWUnit* PartyMember[5]; extern WoWUnit* Leader;
extern time_t current_time;
extern Position playerLastPos;