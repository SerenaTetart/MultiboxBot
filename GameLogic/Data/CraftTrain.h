#include "../Game.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

enum class ProfessionId {
    Leatherworking,
    Blacksmithing,
    Mining,
    Alchemy,
    Tailoring,
    Enchanting
};

struct CraftCostEntry {
    int item_id;
    int item_quantity;
};

struct BuyableItem {
    int item_id;
    int unit_price;
};

struct Craft {
    int crafted_id;
    int required_level;
    int max_level;
    std::vector<CraftCostEntry> resources;
    int spell_id = 0;
    int required_rod = 0;
    int target_slot = 0;
};

using Profession = std::vector<Craft>;

struct ProfessionToCheck {
    ProfessionId id;
    const Profession& crafts;
    int level;
};

struct CraftPlan {
    ProfessionId profession_id;
    int crafted_id;
    int quantity;
    std::vector<CraftCostEntry> to_buy;
    std::int64_t total_cost;
};

inline const std::vector<BuyableItem> buyableItems = {
    { 2320, 10 }, // Coarse Thread
    { 4289, 50 }, // Salt
    { 2321, 100 }, // Fine Thread
    { 4340, 350 }, // Gray Dye
    { 2605, 100 }, // Green Dye
    { 2325, 1000 }, // Black Dye
    { 4291, 500 }, // Silken Thread
    { 3857, 500 }, // Coal
    { 2324, 25 }, // Bleach
    { 2604, 50 }, // Red Dye
    { 6260, 50 }, // Blue Dye
    { 8343, 2000 }, // Heavy Silken Thread
    { 14341, 5000 }, // Rune Thread
    { 3371, 20 }, // Empty Vial
    { 3372, 200 }, // Leaded Vial
    { 8925, 2500 }, // Crystal Vial
    { 6217, 124 }, // Copper Rod
    { 4470, 38 }, // Simple Wood
    { 11291, 4500 }, // Star Wood
};

inline const Profession leatherWorkingCrafts = {
    // Always
    { 2318, 1, 999,{ { 2934, 3 } } }, // Light Leather
    { 4231, 35, 125,{ { 783, 1 },{ 4289, 1 } } }, // Cured Light Hide
    { 4233, 100, 200,{ { 4232, 1 },{ 4289, 1 } } }, // Cured Medium Hide
    { 2319, 130, 999,{ { 2318, 4 } } }, // Medium Leather
    { 4236, 150, 250,{ { 4235, 1 }, { 4289, 3 } } }, // Cured Heavy Hide
    { 4234, 182, 999,{ { 2319, 5 } } }, // Heavy Leather
    { 8172, 200, 300,{ { 8169, 1 }, { 8150, 1 } } }, // Cured Thick Hide
    { 4304, 220, 999,{ { 4234, 6 } } }, // Thick Leather
    { 15407, 250, 999,{ { 8171, 1 }, { 15409, 1 } } }, // Cured Rugged Hide (Salt Shaker required)
    { 8170, 265, 999,{ { 4304, 6 } } }, // Rugged Leather

    // Level up
    { 2304, 1, 45,{ { 2318, 1 } } }, // Light Armor Kit
    { 2302, 45, 55,{ { 2318, 2 },{ 2320, 1 } } }, // Handstitched Leather Boots
    { 4239, 55, 95,{ { 2318, 3 },{ 2320, 2 } } }, // Embossed Leather Gloves
    { 4246, 95, 125,{ { 2318, 6 },{ 2320, 2 } } }, // Fine Leather Belt
    { 2319, 100, 105,{ {2318, 4} } }, // Medium Leather (level)
    { 2313, 100, 115,{ { 2319, 4 },{ 2320, 1 } } }, // Medium Armor Kit
    { 4243, 125, 130,{ { 2318, 6 },{ 2320, 4 },{ 4231, 3 } } }, // Fine Leather Tunic
    { 2315, 115, 135,{ { 2319, 4 },{ 2321, 2 },{ 4340, 1 } } }, // Dark Leather Boots
    { 4234, 150, 155,{ { 2319, 5 } } }, // Heavy Leather (level)
    { 4251, 130, 170,{ { 2319, 4 },{ 2321, 1 },{ 4233, 1 } } }, // Hillman's Shoulders
    { 2314, 135, 150,{ { 2319, 10 },{ 2321, 2 },{ 4231, 2 } } }, // Toughened Leather Armor
    { 2316, 135, 145,{ { 2319, 8 },{ 2321, 1 },{ 4340, 1 } } }, // Dark Leather Cloak
    { 5961, 145, 150,{ { 2319, 12 }, { 2321, 1 },{ 4340, 1 } } }, // Dark Leather Pants
    { 4247, 145, 182,{ { 2319, 14 }, { 2321, 4 } } }, // Hillman's Leather Gloves
    { 4265, 150, 180,{ { 4234, 5 }, { 2321, 1 } } }, // Heavy Armor Kit
    { 4259, 180, 190,{ { 4234, 6 }, { 2321, 1 }, { 4236, 2 },{ 2605, 1 } } }, // Green Leather Bracers
    { 5966, 190, 220,{ { 4234, 6 }, { 4291, 1 }, { 4236, 1 } } }, // Guardian Gloves
    { 7378, 180, 200,{ { 4234, 16 }, { 2325, 1 },{ 4291, 2 } } }, // Dusky Bracers
    { 8173, 200, 225,{ { 4304, 5 }, { 4291, 1 } } }, // Thick Armor Kit
    { 8176, 205, 235,{ { 4304, 5 }, { 4291, 2 } } }, // Nightscape Headband
    { 8193, 230, 250,{ { 4304, 14 }, { 4291, 4 } } }, // Nightscape Pants
    { 8197, 235, 265,{ { 4304, 16 }, { 8343, 2 } } }, // Nightscape Boots
};

inline const Profession blacksmithCrafts = {
    // Always
    { 3470, 25, 999,{ { 2835, 2 } } }, // Rough Grinding Stone
    { 3478, 75, 999,{ { 2836, 2 } } }, // Coarse Grinding Stone
    { 3486, 125, 999,{ { 2838, 3 } } }, // Heavy Grinding Stone
    { 7966, 200, 999,{ { 7912, 4 } } }, // Solid Grinding Stone
    { 12644, 250, 999,{ { 12365, 4 } } }, // Dense Grinding Stone

    // Level up
    { 2862, 1, 25, { { 2835, 1 } } }, // Rough Sharpening Stone
    { 2853, 15, 20, { { 2840, 2 } } }, // Copper Bracers
    { 2852, 20, 50, { { 2840, 4 } } }, // Copper Chain Pants
    { 2863, 65, 75, { { 2836, 1 } } }, // Coarse Sharpening Stone
    { 2851, 50, 75, { { 2840, 6 } } }, // Copper Chain Belt
    { 3472, 75, 80, { { 2840, 8 }, { 3470, 2 } } }, // Runed Copper Gauntlets
    { 2857, 80, 100, { { 2840, 10 } } }, // Runed Copper Belt
    { 6338, 100, 105, { { 2842, 1 }, { 3470, 2 } } }, // Silver Rod
    { 6350, 95, 105, { { 2841, 6 }, { 3470, 6 } } }, // Rough Bronze Boots
    { 15869, 105, 120, { { 2842, 1 }, { 3470, 1 } } }, // Silver Skeleton Key
    { 3480, 110, 140, { { 2841, 5 }, { 1210, 1 }, { 3478, 1 } } }, // Rough Bronze Shoulders
    { 3483, 160, 165, { { 2841, 8 }, { 3478, 2 },{ 2842, 1 } } }, // Silvered Bronze Gauntlets
    { 3482, 150, 175, { { 2841, 6 },{ 3478, 2 },{ 2842, 1 } } }, // Silvered Bronze Boots
    { 2868, 120, 165, { { 2841, 5 }, { 3478, 2 } } }, // Patterned Bronze Bracers
    { 2865, 105, 160, { { 2841, 6 } } }, // Rough Bronze Leggings
    { 3842, 155, 165, { { 3575, 8 }, { 3486, 1 }, { 2605, 1 } } }, // Green Iron Leggings
    { 3835, 165, 205, { { 3575, 6 }, { 2605, 1 } } }, // Green Iron Bracers
    { 6040, 185, 222,{ { 3859, 5 }, { 3486, 2 } } }, // Golden Scale Bracers
    { 11144, 200, 210, { { 6037, 1 }, { 3486, 1 } } }, // Truesilver Rod
    { 7920, 205, 225, { { 3860, 12 } } }, // Mithril Scale Pants
    { 7922, 215, 235,{ { 3859, 14 }, { 7966, 1 } } }, // Steel Plate Helm
    { 7931, 230, 250,{ { 3860, 10 }, { 4338, 6 } } }, // Mithril Coif
    { 7933, 235, 275,{ { 3860, 14 }, { 4304, 4 } } }, // Heavy Mithril Boots
    { 15872, 275, 280,{ { 12360, 1 }, { 12644, 1 } } }, // Arcanite Skeleton Key
};

inline const Profession miningCrafts = {
    { 2840, 1, 999,{ { 2770, 1 } } }, // Copper Bar
    { 3576, 65, 999,{ { 2771, 1 } } }, // Tin Bar
    { 2841, 65, 999,{ { 2840, 1 }, { 3576, 1 } } }, // Bronze Bar
    { 2842, 75, 999,{ { 2775, 1 } } }, // Silver Bar
    { 3575, 125, 999,{ { 2772, 1 } } }, // Iron Bar
    { 3577, 155, 999,{ { 2776, 1 } } }, // Gold Bar
    { 3859, 165, 999,{ { 3575, 1 }, { 3857, 1 } } }, // Steel Bar
    { 3860, 175, 999,{ { 3858, 1 } } }, // Mithril Bar
    { 6037, 230, 999,{ { 7911, 1 } } }, // Truesilver Bar
    { 12359, 250, 999,{ { 10620, 1 } } }, // Thorium Bar
};

inline const Profession tailoringCrafts = {
    // Always
    { 2996, 1, 999,{ { 2589, 2 } } }, // Bolt of Linen Cloth
    { 2997, 75, 999,{ { 2592, 3 } } }, // Bolt of Woolen Cloth
    { 4305, 125, 999,{ { 4306, 4 } } }, // Bolt of Silk Cloth
    { 4339, 175, 999,{ { 4338, 5 } } }, // Bolt of Mageweave
    { 14048, 250, 999,{ { 14047, 5 } } }, // Bolt of Runecloth

    // Level up
    { 4344, 1, 30,{ { 2996, 1 }, { 2320, 1 } } }, // Brown Linen Shirt
    { 6238, 30, 70,{ { 2996, 3 }, { 2320, 1 } } }, // Brown Linen Robe
    { 4309, 70, 130,{ { 2996, 4 }, { 2321, 2 } } }, // Handstitched Linen Britches
    { 4310, 85, 110,{ { 2997, 3 }, { 2321, 1 } } }, // Heavy Woolen Gloves
    { 4316, 110, 170,{ { 2997, 5 }, { 2321, 4 } } }, // Heavy Woolen Pants
    { 7046, 140, 160,{ { 4305, 4 },{ 6260, 2 },{ 2321, 3 } } }, // Azure Silk Pants
    { 4324, 160, 180,{ { 4305, 5 },{ 6260, 4 } } }, // Azure Silk Vest
    { 7057, 180, 215,{ { 4305, 5 },{ 4291, 2 } } }, // Green Silken Shoulders
    { 9999, 205, 220,{ { 4339, 2 },{ 4291, 3 } } }, // Black Mageweave Leggings
    { 10003, 220, 230,{ { 4339, 2 },{ 8343, 2 } } }, // Black Mageweave Gloves
    
    { 10024, 230, 260,{ { 4339, 3 }, { 8343, 2 } } }, // Black Mageweave Headband
    { 13856, 255, 300,{ { 14048, 3 }, { 14341, 1 } } }, // Runecloth Belt
};

inline const Profession alchemyCrafts = {
    // Level up
    { 118, 1, 60,{ { 2447, 1 }, { 765, 1 }, { 3371, 1 } } }, // Minor Healing Potion
    { 858, 55, 105,{ { 118, 1 }, { 2450, 1 } } }, // Lesser Healing Potion
    { 3383, 90, 110,{ { 785, 1 }, { 2450, 2 }, { 3371, 1 } } }, // Elixir of Wisdom
    { 929, 110, 150,{ { 2453, 1 }, { 2450, 1 }, { 3372, 1 } } }, // Healing Potion
    { 3385, 120, 155,{ { 785, 1 }, { 3820, 1 }, { 3371, 1 } } }, // Lesser Mana Potion
    { 1710, 155, 185,{ { 3357, 1 }, { 3356, 1 }, { 3372, 1 } } }, // Greater Healing Potion
    { 8949, 185, 210,{ { 3820, 1 }, { 3821, 1 }, { 3372, 1 } } }, // Elixir of Agility
    { 8951, 195, 215,{ { 3355, 1 }, { 3821, 1 }, { 3372, 1 } } }, // Elixir of Greater Defense
    { 3928, 215, 230,{ { 8838, 1 }, { 3358, 1 }, { 8925, 1 } } }, // Superior Healing Potion
    { 9154, 230, 250,{ { 8836, 1 }, { 8925, 1 } } }, // Elixir of Detect Undead
    { 9187, 240, 265,{ { 8838, 1 }, { 3821, 1 }, { 8925, 1 } } }, // Elixir of Greater Agility
    { 9233, 250, 300,{ { 8846, 2 }, { 8925, 1 } } }, // Elixir of Detect Demon
};

inline const Profession enchantingCrafts = {
    // Always
    { 6218, 1, 999,{ { 6217, 1 }, { 10940, 1 }, { 10938, 1 } }, 7421, 0, 0 }, // Runed Copper Rod
    { 6339, 100, 999,{ { 6338, 1 }, { 10940, 6 }, { 10939, 3 }, { 1210, 1 } }, 7795, 0, 0 }, // Runed Silver Rod
    { 11130, 150, 999,{ { 11128, 1 }, { 5500, 1 }, { 11082, 2 }, { 11083, 2 } }, 13628, 0, 0 }, // Runed Golden Rod
    { 11145, 200, 999,{ { 11144, 1 }, { 7971, 1 }, { 11135, 2 }, { 11137, 2 } }, 13702, 0, 0 }, // Runed Truesilver Rod

    // Level up
    { 11287, 10, 70,{ { 4470, 1 },{ 10938, 1 } }, 14293, 6218, 0 }, // Lesser Magic Wand
    { 7418, 1, 70,{ { 10940, 1 } }, 0, 6218, 9 }, // Enchant Bracer - Minor Health
    { 11288, 70, 130,{ { 4470, 1 },{ 10939, 1 } }, 14807, 6218, 0 }, // Greater Magic Wand
    { 7457, 50, 110,{ { 10940, 3 } }, 0, 6218, 9 }, // Enchant Bracer - Minor Stamina
    { 13501, 130, 160,{ { 11083, 2 } }, 0, 6339, 9 }, // Enchant Bracer - Lesser Stamina
    { 13640, 160, 180,{ { 11083, 3 } }, 0, 11130, 5 }, // Enchant Chest - Greater Health
    { 13642, 165, 185,{ { 11134, 1 } }, 0, 11130, 9 }, // Enchant Bracer - Spirit
    { 13661, 180, 205,{ { 11137, 1 } }, 0, 11130, 9 }, // Enchant Bracer - Strength
    { 13746, 205, 225,{ { 11137, 3 } }, 0, 11145, 15 }, // Enchant Cloak - Greater Defense
    { 13836, 215, 235,{ { 11137, 5 } }, 0, 11145, 8 }, // Enchant Boots - Stamina
    { 13858, 220, 240,{ { 11137, 6 } }, 0, 11145, 5 }, // Enchant Chest - Superior Health
    { 13939, 240, 260,{ { 11176, 2 }, { 11175, 1 } }, 0, 11145, 9 }, // Enchant Bracer - Greater Strength
    { 13948, 250, 300,{ { 11178, 2 }, { 8153, 2 } }, 0, 11145, 10 }, // Enchant Gloves - Minor Haste
};