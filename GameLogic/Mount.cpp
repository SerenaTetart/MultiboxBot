#include "Game.h"
#include "Functions.h"
#include "FunctionsLua.h"
#include <iostream>

static const int FastMountItemsID[38] = {
	// Alliance
	18777, 18776, 18778,				// Human
	18786, 18787, 18785,				// Dwarf
	18766, 18767, 18902,				// Night Elf
	18772, 18773, 18774,				// Gnome
	18241, 18244, 18242, 18243, 19030,	// PvP
	13086,								// Quest
	// Horde
	18796, 18798, 18797,				// Orc
	13334, 18791,						// Undead
	18794, 18795, 18793,				// Tauren
	18788, 18789, 18790,				// Troll
	18245, 18248, 18247, 18246, 19029,	// PvP
	// Neutral
	13335, 19872, 19902,				// Rare drops
	21176								// LEGENDARY
};

// Items
static const int SlowMountItemsID[25] = {
	// Alliance
	2414, 5656, 5655, 2411,		// Human
	5872, 5864, 5873,			// Dwarf
	8632, 8631, 8629,			// Night Elf
	8595, 13321, 8563, 13322,	// Gnome
	// Horde
	5668, 5665, 1132,			// Orc
	13332, 13333, 13331,		// Undead
	15290, 15277,				// Tauren
	8588, 8591, 8592,			// Troll
};

// Buffs
static const int MountBuffsID[68] = {
	// Alliance
	470, 472, 458, 6648, 23229, 23227, 23228,			// Human
	6899, 6777, 6898, 23238, 23239, 23240,				// Dwarf
	10789, 8394, 10793, 23221, 23219, 23338,			// Night Elf
	10969, 17453, 10873, 17454, 23225, 23223, 23222,	// Gnome
	22717, 22720, 22723, 22719, 23510,					// PvP
	17229,												// Quest
	// Horde
	6654, 6653, 580, 23250, 23252, 23251,				// Orc
	17463, 17464, 17462, 17465, 23246,					// Undead
	18990, 18989, 23249, 23248, 23247,					// Tauren
	8395, 10796, 10799, 23241, 23242, 23243,			// Troll
	22724, 22722, 22718, 22721, 23509,					// PvP
	// Neutral
	13819, 23214, 5784, 23161, 783,						// Class
	17481, 24242, 24252,								// Rare drops
	26656												// LEGENDARY
};

void Game::UseMount() {
	if (localPlayer->isMounted) return;
	for (const auto& item : virtualInventory) {
		// Fast
		for (unsigned int z = 0; z < std::size(FastMountItemsID); z++) {
			if (get<2>(item) == FastMountItemsID[z]) {
				FunctionsLua::UseItem(FastMountItemsID[z]);
				return;
			}
		}
	}
	if (localPlayer->className == "Paladin") {
		if (FunctionsLua::IsSpellReady("Summon Charger")) {
			FunctionsLua::CastSpellByName("Summon Charger");
			return;
		}
		else if (FunctionsLua::IsSpellReady("Summon Warhorse")) {
			FunctionsLua::CastSpellByName("Summon Warhorse");
			return;
		}
	}
	else if (localPlayer->className == "Warlock") {
		if (FunctionsLua::IsSpellReady("Summon Dreadsteed")) {
			FunctionsLua::CastSpellByName("Summon Dreadsteed");
			return;
		}
		else if (FunctionsLua::IsSpellReady("Summon Felsteed")) {
			FunctionsLua::CastSpellByName("Summon Felsteed");
			return;
		}
	}
	for (const auto& item : virtualInventory) {
		// Slow
		for (unsigned int z = 0; z < std::size(SlowMountItemsID); z++) {
			if (get<2>(item) == SlowMountItemsID[z]) {
				FunctionsLua::UseItem(SlowMountItemsID[z]);
				return;
			}
		}
	}
	if (localPlayer->className == "Druid" && FunctionsLua::IsSpellReady("Travel Form")) {
		FunctionsLua::CastSpellByName("Travel Form");
		return;
	}
}

void Game::Dismount() {
	if (!localPlayer->isMounted) return;
	int buff_id = localPlayer->hasBuff(MountBuffsID, 67);
	if (buff_id != 0) {
		Functions::CancelPlayerBuff(buff_id);
		localPlayer->isMounted = false;
	}
}