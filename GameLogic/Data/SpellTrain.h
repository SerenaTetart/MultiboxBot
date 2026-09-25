#pragma once

#include "../Game.h"

#include <cstddef>
#include <cstdint>

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
    std::uint32_t spellId;
    std::uint32_t cost;
    std::uint8_t level;
};

struct SpellTrainList {
    const SpellTrainEntry* data;
    std::size_t size;

    const SpellTrainEntry* begin() const noexcept { return data; }
    const SpellTrainEntry* end() const noexcept { return data + size; }
};

inline constexpr SpellTrainEntry druidSpells[] = {
    { 774, 100, 4 },
    { 467, 100, 6 },
    { 5186, 200, 8 },
    { 5232, 300, 10 },
    { 8936, 800, 12 },
    { 5187, 900, 14 },
    { 1430, 1800, 16 },
    { 8938, 1900, 18 },
    { 768, 2000, 20 },
    { 2090, 3000, 22 },
    { 2782, 4000, 24 },
    { 5189, 4500, 26 },
    { 2091, 5000, 28 },
    { 5234, 6000, 30 },
    { 6778, 8000, 32 },
    { 3627, 10000, 34 },
    { 8941, 11000, 36 },
    { 8903, 12000, 38 },
    { 9634, 14000, 40 },
    { 9750, 16000, 42 },
    { 9758, 18000, 44 },
    { 9839, 20000, 46 },
    { 9856, 22000, 48 },
    { 9884, 23000, 50 },
    { 9840, 26000, 52 },
    { 9857, 28000, 54 },
    { 9889, 30000, 56 },
    { 9841, 32000, 58 },
    { 9858, 34000, 60 }
};

inline constexpr SpellTrainEntry hunterSpells[] = {
    { 1978, 100, 4 },
    { 3044, 100, 6 },
    { 5116, 200, 8 },
    { 13165, 400, 10 },
    { 14281, 600, 12 },
    { 1513, 1200, 14 },
    { 1495, 1800, 16 },
    { 2643, 2000, 18 },
    { 14282, 2200, 20 },
    { 14323, 6000, 22 },
    { 14262, 7000, 24 },
    { 13551, 7000, 26 },
    { 14319, 8000, 28 },
    { 14269, 8000, 30 },
    { 14263, 10000, 32 },
    { 13552, 12000, 34 },
    { 14284, 14000, 36 },
    { 14320, 16000, 38 },
    { 1510, 18000, 40 },
    { 14289, 24000, 42 },
    { 14270, 26000, 44 },
    { 20043, 28000, 46 },
    { 14321, 32000, 48 },
    { 13554, 36000, 50 },
    { 14286, 40000, 52 },
    { 14290, 42000, 54 },
    { 14266, 46000, 56 },
    { 14322, 48000, 58 },
    { 14287, 50000, 60 }
};

inline constexpr SpellTrainEntry mageSpells[] = {
    { 116, 100, 4 },
    { 2136, 100, 6 },
    { 205, 200, 8 },
    { 7300, 400, 10 },
    { 145, 600, 12 },
    { 1460, 900, 14 },
    { 2120, 1500, 16 },
    { 475, 1800, 18 },
    { 12051, 2000, 20 },
    { 2138, 3000, 22 },
    { 2139, 4000, 24 },
    { 120, 5000, 26 },
    { 1461, 7000, 28 },
    { 8438, 8000, 30 },
    { 8422, 10000, 32 },
    { 6117, 13000, 34 },
    { 8427, 13000, 36 },
    { 8439, 14000, 38 },
    { 10138, 15000, 40 },
    { 10156, 18000, 42 },
    { 10185, 23000, 44 },
    { 10201, 26000, 46 },
    { 10215, 28000, 48 },
    { 10160, 32000, 50 },
    { 10186, 35000, 52 },
    { 10202, 36000, 54 },
    { 10181, 38000, 56 },
    { 10161, 40000, 58 },
    { 10187, 42000, 60 }
};

inline constexpr SpellTrainEntry paladinSpells[] = {
    { 20271, 100, 4 },
    { 639, 100, 6 },
    { 853, 100, 8 },
    { 10290, 300, 10 },
    { 19834, 1000, 12 },
    { 19742, 2000, 14 },
    { 25780, 3000, 16 },
    { 20288, 3500, 18 },
    { 643, 4000, 20 },
    { 1026, 4000, 22 },
    { 5588, 5000, 24 },
    { 10298, 6000, 26 },
    { 5614, 9000, 28 },
    { 1042, 11000, 30 },
    { 20306, 12000, 32 },
    { 19852, 13000, 34 },
    { 5615, 14000, 36 },
    { 3472, 16000, 38 },
    { 1032, 20000, 40 },
    { 4987, 21000, 42 },
    { 19853, 22000, 44 },
    { 10328, 24000, 46 },
    { 20356, 26000, 48 },
    { 10292, 28000, 50 },
    { 10838, 34000, 52 },
    { 19854, 40000, 54 },
    { 10301, 42000, 56 },
    { 20357, 44000, 58 },
    { 10293, 46000, 60 }
};

inline constexpr SpellTrainEntry priestSpells[] = {
    { 2052, 100, 4 },
    { 17, 100, 6 },
    { 586, 200, 8 },
    { 2053, 300, 10 },
    { 588, 1000, 12 },
    { 8122, 1200, 14 },
    { 2054, 1600, 16 },
    { 527, 2000, 18 },
    { 9578, 3000, 20 },
    { 2055, 4000, 22 },
    { 1245, 5000, 24 },
    { 6076, 6000, 26 },
    { 6063, 8000, 28 },
    { 6065, 10000, 30 },
    { 9473, 11000, 32 },
    { 6064, 12000, 34 },
    { 988, 14000, 36 },
    { 6078, 16000, 38 },
    { 2060, 18000, 40 },
    { 10898, 22000, 42 },
    { 10927, 24000, 44 },
    { 10963, 26000, 46 },
    { 10937, 28000, 48 },
    { 10941, 30000, 50 },
    { 10946, 38000, 52 },
    { 10900, 40000, 54 },
    { 10929, 42000, 56 },
    { 10965, 44000, 58 },
    { 10952, 46000, 60 }
};

inline constexpr SpellTrainEntry rogueSpells[] = {
    { 53, 100, 4 },
    { 1757, 100, 6 },
    { 6760, 200, 8 },
    { 5171, 300, 10 },
    { 2589, 800, 12 },
    { 1758, 1200, 14 },
    { 6761, 1800, 16 },
    { 8676, 2900, 18 },
    { 1785, 3000, 20 },
    { 8631, 4000, 22 },
    { 6762, 5000, 24 },
    { 8724, 6000, 26 },
    { 6768, 8000, 28 },
    { 408, 10000, 30 },
    { 8623, 12000, 32 },
    { 8725, 14000, 34 },
    { 8721, 16000, 36 },
    { 8633, 18000, 38 },
    { 8624, 20000, 40 },
    { 11267, 27000, 42 },
    { 11273, 29000, 44 },
    { 11293, 31000, 46 },
    { 11299, 33000, 48 },
    { 8643, 35000, 50 },
    { 11303, 46000, 52 },
    { 11294, 48000, 54 },
    { 11300, 50000, 56 },
    { 11269, 52000, 58 },
    { 11281, 54000, 60 }
};

inline constexpr SpellTrainEntry shamanSpells[] = {
    { 8042, 100, 4 },
    { 332, 100, 6 },
    { 529, 100, 8 },
    { 8050, 400, 10 },
    { 2008, 800, 12 },
    { 548, 900, 14 },
    { 8019, 1800, 16 },
    { 8027, 2000, 18 },
    { 8056, 2200, 20 },
    { 8166, 3000, 22 },
    { 8046, 3500, 24 },
    { 943, 4000, 26 },
    { 8038, 6000, 28 },
    { 8232, 7000, 30 },
    { 421, 8000, 32 },
    { 16314, 9000, 34 },
    { 10412, 10000, 36 },
    { 10391, 11000, 38 },
    { 1064, 12000, 40 },
    { 10613, 14000, 42 },
    { 10392, 18000, 44 },
    { 10622, 20000, 46 },
    { 10413, 22000, 48 },
    { 15207, 24000, 50 },
    { 10448, 27000, 52 },
    { 10623, 29000, 54 },
    { 15208, 30000, 56 },
    { 10473, 32000, 58 },
    { 10414, 34000, 60 }
};

inline constexpr SpellTrainEntry warlockSpells[] = {
    { 172, 100, 4 },
    { 1454, 100, 6 },
    { 5782, 200, 8 },
    { 1120, 300, 10 },
    { 705, 800, 12 },
    { 6222, 900, 14 },
    { 1455, 1800, 16 },
    { 5676, 2000, 18 },
    { 1088, 2200, 20 },
    { 6205, 3000, 22 },
    { 5138, 3500, 24 },
    { 1714, 4000, 26 },
    { 710, 5000, 28 },
    { 1949, 6000, 30 },
    { 7646, 7000, 32 },
    { 17920, 8000, 34 },
    { 7641, 9000, 36 },
    { 11711, 10000, 38 },
    { 11733, 11000, 40 },
    { 17921, 11000, 42 },
    { 11659, 11000, 44 },
    { 11699, 13000, 46 },
    { 18647, 15000, 48 },
    { 11734, 18000, 50 },
    { 11660, 20000, 52 },
    { 11684, 22000, 54 },
    { 11689, 23000, 56 },
    { 17923, 24000, 58 },
    { 11661, 26000, 60 }
};

inline constexpr SpellTrainEntry warriorSpells[] = {
    { 100, 100, 4 },
    { 6343, 100, 6 },
    { 1715, 200, 8 },
    { 6546, 400, 10 },
    { 7384, 800, 12 },
    { 1160, 1500, 14 },
    { 694, 2000, 16 },
    { 676, 3000, 18 },
    { 845, 4000, 20 },
    { 5246, 6000, 22 },
    { 5308, 8000, 24 },
    { 1161, 10000, 26 },
    { 871, 11000, 28 },
    { 1464, 12000, 30 },
    { 20658, 14000, 32 },
    { 11554, 16000, 34 },
    { 1680, 18000, 36 },
    { 8205, 20000, 38 },
    { 20660, 22000, 40 },
    { 11550, 32000, 42 },
    { 11555, 34000, 44 },
    { 11578, 36000, 46 },
    { 11580, 42000, 48 },
    { 11609, 44000, 50 },
    { 11551, 54000, 52 },
    { 11556, 56000, 54 },
    { 20662, 58000, 56 },
    { 11581, 60000, 58 },
    { 11610, 62000, 60 }
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
		if(spellTrain.level > localPlayer->level) break;
		bool spellTrained = false;
        for (const auto& spellBook : virtualSpellBook) {
			if(spellBook.id == spellTrain.spellId) {
				spellTrained = true;
				break;
			}
		}
		if(!spellTrained) return false;
    }

    return true;
}