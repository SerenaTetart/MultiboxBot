#pragma once

#include "../Game.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <iostream>

enum class PlayerClass : std::uint8_t {
    Druid,
    Hunter,
    Mage,
    Paladin,
    Priest,
    Rogue,
    Shaman,
    Warlock,
    Warrior
};

struct SpellTrainEntry {
    std::string_view spellName;
    int cost;
    int level;
    int rank;
};

struct SpellTrainList {
    const SpellTrainEntry* data;
    std::size_t size;

    const SpellTrainEntry* begin() const noexcept { return data; }
    const SpellTrainEntry* end() const noexcept { return data + size; }
};

inline constexpr SpellTrainEntry druidSpells[] = {
    { "Moonfire", 100, 4, 1},
    { "Wrath", 100, 6, 2},
    { "Healing Touch", 200, 8, 2},
    { "Moonfire", 300, 10, 2},
    { "Regrowth", 800, 12, 1},
    { "Wrath", 900, 14, 3},
    { "Moonfire", 1800, 16, 3},
    { "Regrowth", 1900, 18, 2},
    { "Healing Touch", 2000, 20, 4},
    { "Moonfire", 3000, 22, 4},
    { "Regrowth", 4000, 24, 3},
    { "Healing Touch", 4500, 26, 5},
    { "Moonfire", 5000, 28, 5},
    { "Wrath", 6000, 30, 5},
    { "Healing Touch", 8000, 32, 6},
    { "Moonfire", 10000, 34, 6},
    { "Regrowth", 11000, 36, 5},
    { "Wrath", 12000, 38, 6},
    { "Moonfire", 14000, 40, 7},
    { "Regrowth", 16000, 42, 6},
    { "Healing Touch", 18000, 44, 8 },
    { "Moonfire", 20000, 46, 8},
    { "Regrowth", 22000, 48, 7},
    { "Healing Touch", 23000, 50, 9},
    { "Moonfire", 26000, 52, 9},
    { "Wrath", 28000, 54, 8},
    { "Healing Touch", 30000, 56, 10},
    { "Moonfire", 32000, 58, 10 },
    { "Regrowth", 34000, 60, 9}
};

inline constexpr SpellTrainEntry hunterSpells[] = {
    { "Serpent Sting", 100, 4, 1},
    { "Arcane Shot", 100, 6, 1 },
    { "Raptor Strike", 200, 8, 2},
    { "Serpent Sting", 400, 10, 2},
    { "Arcane Shot", 600, 12, 2},
    { "Mongoose Bite", 1200, 14, 1},
    { "Raptor Strike", 1800, 16, 3},
    { "Serpent Sting", 2000, 18, 3},
    { "Arcane Shot", 2200, 20, 3},
    { "Scorpid Sting", 6000, 22, 1},
    { "Raptor Strike", 7000, 24, 4},
    { "Serpent Sting", 7000, 26, 4},
    { "Arcane Shot", 8000, 28, 4},
    { "Mongoose Bite", 8000, 30, 2},
    { "Raptor Strike", 10000, 32, 5},
    { "Serpent Sting", 12000, 34, 5},
    { "Arcane Shot", 14000, 36, 5},
    { "Aspect of the Hawk", 16000, 38, 4},
    { "Raptor Strike", 18000, 40, 6},
    { "Serpent Sting", 24000, 42, 6},
    { "Arcane Shot", 26000, 44, 6},
    { "Aspect of the Wild", 28000, 46, 1},
    { "Raptor Strike", 32000, 48, 7},
    { "Serpent Sting", 36000, 50, 7},
    { "Arcane Shot", 40000, 52, 7},
    { "Multi-Shot", 42000, 54, 4 },
    { "Raptor Strike", 46000, 56, 8},
    { "Serpent Sting", 48000, 58, 8},
    { "Arcane Shot", 50000, 60, 8}
};

inline constexpr SpellTrainEntry mageSpells[] = {
    { "Frostbolt", 100, 4, 1},
    { "Fireball", 100, 6, 2},
    { "Frostbolt", 200, 8, 2},
    { "Frost Nova", 400, 10, 1},
    { "Fireball", 600, 12, 3},
    { "Frostbolt", 900, 14, 3},
    { "Arcane Missiles", 1500, 16, 2},
    { "Fireball", 1800, 18, 4},
    { "Frostbolt", 2000, 20, 4},
    { "Fire Blast", 3000, 22, 3},
    { "Fireball", 4000, 24, 5},
    { "Frostbolt", 5000, 26, 5},
    { "Arcane Intellect", 7000, 28, 3},
    { "Fireball", 8000, 30, 6},
    { "Frostbolt", 10000, 32, 6},
    { "Mage Armor", 13000, 34, 1},
    { "Fireball", 13000, 36, 7},
    { "Frostbolt", 14000, 38, 7},
    { "Frost Nova", 15000, 40, 3},
    { "Fireball", 18000, 42, 8},
    { "Frostbolt", 23000, 44, 8},
    { "Fire Blast", 26000, 46, 6},
    { "Fireball", 28000, 48, 9},
    { "Frostbolt", 32000, 50, 9},
    { "Scorch", 35000, 52, 6},
    { "Fireball", 36000, 54, 10},
    { "Frostbolt", 38000, 56, 10},
    { "Mage Armor", 40000, 58, 3},
    { "Fireball", 42000, 60, 11}
};

inline constexpr SpellTrainEntry paladinSpells[] = {
    { "Judgement", 100, 4, 0},
    { "Holy Light", 100, 6, 2},
    { "Hammer of Justice", 100, 8, 1},
    { "Seal of Righteousness", 300, 10, 2},
    { "Seal of the Crusader", 1000, 12, 2},
    { "Holy Light", 2000, 14, 3},
    { "Retribution Aura", 3000, 16, 1},
    { "Seal of Righteousness", 3500, 18, 3},
    { "Flash of Light", 4000, 20, 1},
    { "Holy Light", 4000, 22, 4},
    { "Hammer of Justice", 5000, 24, 2},
    { "Seal of Righteousness", 6000, 26, 4},
    { "Exorcism", 9000, 28, 2},
    { "Holy Light", 11000, 30, 5},
    { "Seal of the Crusader", 12000, 32, 4},
    { "Seal of Righteousness", 13000, 34, 5},
    { "Retribution Aura", 14000, 36, 3},
    { "Holy Light", 16000, 38, 6},
    { "Hammer of Justice", 20000, 40, 3},
    { "Seal of Righteousness", 21000, 42, 6},
    { "Exorcism", 22000, 44, 4},
    { "Holy Light", 24000, 46, 7},
    { "Seal of Wisdom", 26000, 48, 2},
    { "Seal of Righteousness", 28000, 50, 7},
    { "Seal of the Crusader", 34000, 52, 6},
    { "Holy Light", 40000, 54, 8},
    { "Retribution Aura", 42000, 56, 5},
    { "Seal of Righteousness", 44000, 58, 8},
    { "Exorcism", 46000, 60, 6}
};

inline constexpr SpellTrainEntry priestSpells[] = {
    { "Shadow Word: Pain", 100, 4, 1},
    { "Power Word: Shield", 100, 6, 1},
    { "Renew", 200, 8, 1},
    { "Shadow Word: Pain", 300, 10, 2},
    { "Power Word: Shield", 1000, 12, 2},
    { "Renew", 1200, 14, 2},
    { "Heal", 1600, 16, 1},
    { "Shadow Word: Pain", 2000, 18, 3},
    { "Renew", 3000, 20, 3},
    { "Heal", 4000, 22, 2},
    { "Power Word: Shield", 5000, 24, 4},
    { "Shadow Word: Pain", 6000, 26, 4},
    { "Heal", 8000, 28, 3},
    { "Power Word: Shield", 10000, 30, 5},
    { "Renew", 11000, 32, 5},
    { "Shadow Word: Pain", 12000, 34, 5},
    { "Power Word: Shield", 14000, 36, 6},
    { "Renew", 16000, 38, 6},
    { "Greater Heal", 18000, 40, 1},
    { "Shadow Word: Pain", 22000, 42, 6},
    { "Renew", 24000, 44, 7},
    { "Greater Heal", 26000, 46, 2},
    { "Power Word: Shield", 28000, 48, 8},
    { "Shadow Word: Pain", 30000, 50, 7},
    { "Greater Heal", 38000, 52, 3},
    { "Power Word: Shield", 40000, 54, 9},
    { "Renew", 42000, 56, 9},
    { "Shadow Word: Pain", 44000, 58, 8},
    { "Power Word: Shield", 46000, 60, 10}
};

inline constexpr SpellTrainEntry rogueSpells[] = {
    { "Backstab", 100, 4, 1},
    { "Sinister Strike", 100, 6, 2},
    { "Eviscerate", 200, 8, 2},
    { "Slice and Dice", 300, 10, 1},
    { "Backstab", 800, 12, 2},
    { "Sinister Strike", 1200, 14, 2},
    { "Eviscerate", 1800, 16, 3},
    { "Gouge", 2900, 18, 2},
    { "Backstab", 3000, 20, 3},
    { "Sinister Strike", 4000, 22, 3},
    { "Eviscerate", 5000, 24, 4},
    { "Ambush", 6000, 26, 2},
    { "Backstab", 8000, 28, 4},
    { "Sinister Strike", 10000, 30, 5},
    { "Eviscerate", 12000, 32, 5},
    { "Ambush", 14000, 34, 3},
    { "Backstab", 16000, 36, 5},
    { "Sinister Strike", 18000, 38, 6},
    { "Eviscerate", 20000, 40, 6},
    { "Slice and Dice", 27000, 42, 2},
    { "Backstab", 29000, 44, 6},
    { "Sinister Strike", 31000, 46, 7},
    { "Eviscerate", 33000, 48, 7},
    { "Ambush", 35000, 50, 5},
    { "Backstab", 46000, 52, 7},
    { "Sinister Strike", 48000, 54, 8},
    { "Eviscerate", 50000, 56, 8},
    { "Ambush", 52000, 58, 6},
    { "Backstab", 54000, 60, 8}
};

inline constexpr SpellTrainEntry shamanSpells[] = {
    { "Earth Shock", 100, 4, 1 },
    { "Healing Wave", 100, 6, 2 },
    { "Earth Shock", 100, 8, 2 },
    { "Flame Shock", 400, 10, 1 },
    { "Healing Wave", 800, 12, 3},
    { "Earth Shock", 900, 14, 3},
    { "Lightning Shield", 1800, 16, 2},
    { "Healing Wave", 2000, 18, 4},
    { "Frost Shock", 2200, 20, 1},
    { "Cure Disease", 3000, 22, 0},
    { "Earth Shock", 3500, 24, 4},
    { "Lightning Bolt", 4000, 26, 5},
    { "Flame Shock", 6000, 28, 3},
    { "Windfury Weapon", 7000, 30, 1},
    { "Healing Wave", 8000, 32, 6},
    { "Frost Shock", 9000, 34, 2},
    { "Earth Shock", 10000, 36, 5},
    { "Lightning Bolt", 11000, 38, 7},
    { "Healing Wave", 12000, 40, 7},
    { "Grace of Air Totem", 14000, 42, 1},
    { "Lightning Bolt", 18000, 44, 8},
    { "Frost Shock", 20000, 46, 3},
    { "Earth Shock", 22000, 48, 6},
    { "Lightning Bolt", 24000, 50, 9},
    { "Flame Shock", 27000, 52, 5},
    { "Chain Heal", 29000, 54, 3},
    { "Healing Wave", 30000, 56, 9},
    { "Frost Shock", 32000, 58, 4},
    { "Earth Shock", 34000, 60, 7}
};

inline constexpr SpellTrainEntry warlockSpells[] = {
    { "Corruption", 100, 4, 1},
    { "Shadow Bolt", 100, 6, 2},
    { "Curse of Agony", 200, 8, 1},
    { "Drain Soul", 300, 10, 1},
    { "Shadow Bolt", 800, 12, 3},
    { "Corruption", 900, 14, 2},
    { "Life Tap", 1800, 16, 2},
    { "Curse of Agony", 2000, 18, 2},
    { "Shadow Bolt", 2200, 20, 4},
    { "Drain Life", 3000, 22, 2},
    { "Corruption", 3500, 24, 3},
    { "Life Tap", 4000, 26, 3},
    { "Shadow Bolt", 5000, 28, 5},
    { "Drain Life", 6000, 30, 3 },
    { "Fear", 7000, 32, 2},
    { "Corruption", 8000, 34, 4},
    { "Shadow Bolt", 9000, 36, 6},
    { "Curse of Agony", 10000, 38, 4},
    { "Immolate", 11000, 40, 5},
    { "Death Coil", 11000, 42, 1},
    { "Corruption", 11000, 44, 5},
    { "Life Tap", 13000, 46, 5},
    { "Curse of Agony", 15000, 48, 5},
    { "Immolate", 18000, 50, 6 },
    { "Shadow Bolt", 20000, 52, 8},
    { "Corruption", 22000, 54, 6},
    { "Life Tap", 23000, 56, 6},
    { "Curse of Agony", 24000, 58, 6},
    { "Corruption", 26000, 60, 7}
};

inline constexpr SpellTrainEntry warriorSpells[] = {
    { "Charge", 100, 4, 1 },
    { "Thunder Clap", 100, 6, 1 },
    { "Hamstring", 200, 8, 1},
    { "Rend", 400, 10, 2},
    { "Overpower", 800, 12, 1},
    { "Revenge", 1500, 14, 1},
    { "Heroic Strike", 2000, 16, 3},
    { "Thunder Clap", 3000, 18, 2},
    { "Rend", 4000, 20, 3},
    { "Sunder Armor", 6000, 22, 2},
    { "Execute", 8000, 24, 1},
    { "Charge", 10000, 26, 2},
    { "Overpower", 11000, 28, 2},
    { "Rend", 12000, 30, 4},
    { "Execute", 14000, 32, 2 },
    { "Revenge", 16000, 34, 3},
    { "Mocking Blow", 18000, 36, 3},
    { "Slam", 20000, 38, 2},
    { "Rend", 22000, 40, 5},
    { "Intercept", 32000, 42, 2},
    { "Overpower", 34000, 44, 3 },
    { "Charge", 36000, 46, 3},
    { "Execute", 42000, 48, 4},
    { "Cleave", 44000, 50, 4},
    { "Intercept", 54000, 52, 3},
    { "Revenge", 56000, 54, 5},
    { "Execute", 58000, 56, 5},
    { "Pummel", 60000, 58, 2},
    { "Rend", 62000, 60, 7}
};

constexpr SpellTrainList spellTrainDatabase[] = {
    { druidSpells, sizeof(druidSpells) / sizeof(druidSpells[0]) },
    { hunterSpells, sizeof(hunterSpells) / sizeof(hunterSpells[0]) },
    { mageSpells, sizeof(mageSpells) / sizeof(mageSpells[0]) },
    { paladinSpells, sizeof(paladinSpells) / sizeof(paladinSpells[0]) },
    { priestSpells, sizeof(priestSpells) / sizeof(priestSpells[0]) },
    { rogueSpells, sizeof(rogueSpells) / sizeof(rogueSpells[0]) },
    { shamanSpells, sizeof(shamanSpells) / sizeof(shamanSpells[0]) },
    { warlockSpells, sizeof(warlockSpells) / sizeof(warlockSpells[0]) },
    { warriorSpells, sizeof(warriorSpells) / sizeof(warriorSpells[0]) }
};

inline SpellTrainList GetSpellTrainList(const std::string& className) noexcept {
	PlayerClass classPlayer;
	if (className == "Druid") classPlayer = PlayerClass::Druid;
	else if (className == "Hunter") classPlayer = PlayerClass::Hunter;
	else if (className == "Mage") classPlayer = PlayerClass::Mage;
	else if (className == "Paladin") classPlayer = PlayerClass::Paladin;
	else if (className == "Priest") classPlayer = PlayerClass::Priest;
	else if (className == "Rogue") classPlayer = PlayerClass::Rogue;
	else if (className == "Shaman") classPlayer = PlayerClass::Shaman;
	else if (className == "Warlock") classPlayer = PlayerClass::Warlock;
	else if (className == "Warrior") classPlayer = PlayerClass::Warrior;

    return spellTrainDatabase[static_cast<std::size_t>(classPlayer)];
}

inline bool HasSpellToTrain() {
    const SpellTrainList list = GetSpellTrainList(localPlayer->className);

    for (const SpellTrainEntry& spellTrain : list) {
		if(spellTrain.level > localPlayer->level || localPlayer->money < spellTrain.cost) break;
		bool spellTrained = false;
        for (const auto& spellBook : virtualSpellBook) {
			if(spellBook.name == spellTrain.spellName && spellBook.rank >= spellTrain.rank) {
				spellTrained = true;
				break;
			}
		}
        if (!spellTrained) return true;
    }

    return false;
}