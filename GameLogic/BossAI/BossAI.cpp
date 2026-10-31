#include "BossAI.h"
#include <algorithm>
#include <array>

bool BossAI::BossAIAction() {
	bool result = false;
	if (!Combat) {
		if (Moving == MoveTypes::BossMechanic) {
			Moving = MoveTypes::NotMoving;
			ThreadSynchronizer::pressKey(0x28);
			ThreadSynchronizer::releaseKey(0x28);
		}
		return false;
	}
	else if (mapID == 90 && localPlayer->zoneID == 721) {
		// Gnomeregan
		result = BossAI::Gnomeregan();
	}
	else if (mapID == 189 && localPlayer->zoneID == 796) {
		// Scarlet Armory
		result = BossAI::ScarletMonastery();
	}
    else if (mapID == 349 && localPlayer->zoneID == 2100) {
        // Maraudon
        result = BossAI::Maraudon();
    }
    else if (mapID == 70 && localPlayer->zoneID == 1337) {
        // Uldaman
        result = BossAI::Uldaman();
    }

	if (result == false && Moving == MoveTypes::BossMechanic) {
		Moving = MoveTypes::NotMoving;
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	return result;
}
// A mapId/zoneId of 0 means "all maps/zones". An entryID of 0 is disabled.
namespace {
    struct NpcDangerRule {
        int EntryId;
        unsigned int MapId;
        unsigned int ZoneId;
        float Radius;
        bool OnlyWhenDead;
        bool OnlyWhenInCombat;
    };

    constexpr std::array<NpcDangerRule, 3> DangerRules{{
        { 12222, 349, 2100, 10.0f, false, true}, // Maraudon: Creeping Sludge
        { 12221, 349, 2100,  7.0f, true, false }, // Maraudon: dead Noxious Slime
        { 7226, 70, 1337, 7.0f, false, true } // Uldaman: Sand Storm
    }};
}

void BossAI::UpdateDangerZones() {
    std::vector<DangerZone> zones;
    zones.reserve(ListUnits.size());
    for (const auto& unit : ListUnits) {
        if (unit.objectType != Unit) continue;

        float radius = 0.0f;
        for (const auto& rule : DangerRules) {
            if (rule.EntryId == 0 || rule.EntryId != unit.entryID) continue;
            if (rule.MapId != 0 && rule.MapId != static_cast<unsigned int>(mapID)) continue;
            if (rule.ZoneId != 0 && rule.ZoneId != static_cast<unsigned int>(localPlayer->zoneID)) continue;
            if (rule.OnlyWhenDead && !unit.isdead) continue;
            if (!rule.OnlyWhenDead && unit.isdead) continue;
            if (rule.OnlyWhenInCombat && !(unit.flags & UNIT_FLAG_IN_COMBAT)) continue;
            radius = (std::max)(radius, rule.Radius);
        }

        if (radius > 0.0f)
            zones.push_back({unit.Guid, static_cast<unsigned int>(mapID), unit.position, radius});
    }
    Navigation::SetNpcDangerZones(static_cast<unsigned int>(mapID), zones);
}
