#include "../Game.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

enum class ProfessionId {
    Leatherworking,
    Blacksmithing
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
    { 2320, 1000 }
};

inline const Profession leatherWorkingCrafts = {
    // Always
    { 2318, 1, 999,{ { 2934, 3 } } }, // Light Leather

    // Level up
    { 2304, 1, 55,{ { 2318, 1 } } }, // Light Armor Kit
    { 3756, 55, 95,{ { 2318, 3 } } }, // Embossed Leather Gloves
    { 3763, 95, 100,{ { 2318, 6 } } } // Fine Leather Belt
};

inline const Profession blacksmithCrafts = {
    // To do...
};

inline std::int64_t GetItemCount(int item_id) {
    std::int64_t count = 0;
    for (const auto& item : virtualInventory)
        if (item.id == item_id) count += item.count;
    return count;
}

inline const BuyableItem* FindBuyableItem(int item_id) {
    for (const auto& item : buyableItems)
        if (item.item_id == item_id) return &item;
    return nullptr;
}

inline std::optional<CraftPlan> TryPlanCraftBatch(ProfessionId profession_id, const Craft& craft, int max_crafts) {
    struct ResourceState {
        int item_id;
        int quantity;
        int unit_price;
        std::int64_t available;
    };

    std::vector<ResourceState> resources;
    resources.reserve(craft.resources.size());

    for (const auto& resource : craft.resources) {
        if (resource.item_quantity <= 0) return std::nullopt;

        const auto* buyable = FindBuyableItem(resource.item_id);
        resources.push_back({
            resource.item_id,
            resource.item_quantity,
            buyable ? buyable->unit_price : -1,
            GetItemCount(resource.item_id)
        });
    }

    CraftPlan plan{ profession_id, craft.crafted_id, 0,{}, 0 };
    plan.to_buy.reserve(resources.size());

    for (int amount = max_crafts; amount >= 1; --amount) {
        plan.quantity = amount;
        plan.total_cost = 0;
        plan.to_buy.clear();
        bool possible = true;

        for (const auto& resource : resources) {
            std::int64_t required = static_cast<std::int64_t>(resource.quantity) * amount;
            std::int64_t missing = required - resource.available;
            if (missing <= 0) continue;

            if (resource.unit_price < 0 || missing >(std::numeric_limits<int>::max)()) {
                possible = false;
                break;
            }

            if (resource.unit_price > 0 && missing > (localPlayer->money - plan.total_cost) / resource.unit_price) {
                possible = false;
                break;
            }

            plan.to_buy.push_back({ resource.item_id, static_cast<int>(missing) });
            plan.total_cost += missing * resource.unit_price;
        }

        if (possible) return plan;
    }

    return std::nullopt;
}

inline std::optional<CraftPlan> GetCraftPlan(int max_crafts = 20) {
    if (!localPlayer || max_crafts <= 0) return std::nullopt;

    const std::array professions = {
        ProfessionToCheck{ ProfessionId::Leatherworking, leatherWorkingCrafts, localPlayer->leatherworkingLevel },
        ProfessionToCheck{ ProfessionId::Blacksmithing, blacksmithCrafts, localPlayer->blacksmithingLevel }
    };

    for (const auto& [id, crafts, level] : professions) {
        if (level <= 0) continue;

        for (const auto& craft : crafts) {
            if (level < craft.required_level || level >= craft.max_level) continue;

            int batch_limit = (std::min)(max_crafts, craft.max_level - level);
            auto plan = TryPlanCraftBatch(id, craft, batch_limit);
            if (plan) return plan;
        }
    }

    return std::nullopt;
}