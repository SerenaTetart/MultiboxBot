#include "BossAI.h"
#include <algorithm>
#include <array>
#include <cmath>

bool BossAI::PositionBehind(WoWUnit* boss) {
    const Position& start = localPlayer->position;
    const float reach = localPlayer->combatReach + boss->combatReach;
    const float distance = start.DistanceTo(boss->position);
    if (distance > reach + 6.0f) return false;

    if (distance <= reach + 0.3f && start.DistanceTo2D(boss->position) > 0.1f && localPlayer->isBehind(boss)) {
        if (Moving == MoveTypes::GoToTarget) Functions::StopMovement();
        return true;
    }

    constexpr float PI = 3.14159265358979323846f;
    const float angle = boss->facing + PI + (static_cast<int>(localPlayer->Guid % 3ULL) - 1) * 0.12f;
    const float range = (std::max)(1.5f, reach - 0.45f);
    Position destination(boss->position.X + std::cos(angle) * range, boss->position.Y + std::sin(angle) * range, boss->position.Z);

    const bool targetSwimming = (boss->movement_flags & MOVEFLAG_SWIMMING) != 0;
    if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING) && !targetSwimming) {
        ThreadSynchronizer::RunOnMainThread([&destination]() {
            destination = Functions::ProjectPos(destination);
        });
        if (std::abs(destination.Z - boss->position.Z) > 2.0f) return false;
    }
    if (start.DistanceTo(destination) < 2.0f || Navigation::IsPositionDangerous(mapID, destination)) return false;

    bool started = false;
    const unsigned long long guid = boss->Guid;
    ThreadSynchronizer::RunOnMainThread([destination, targetSwimming, guid, &started]() {
        if (!localPlayer || !targetUnit || targetUnit->Guid != guid || Moving == MoveTypes::BossMechanic || Moving == MoveTypes::RunAway) return;
        Functions::MoveTo(destination, MoveTypes::GoToTarget, true, targetSwimming);
        started = Moving == MoveTypes::GoToTarget && Navigation::HasCurrentMovement();
    });
    return started;
}

bool BossAI::BossAIAction() {
	bool result = false;
	if (!Combat || localPlayer->isCrowdControlled()) {
		if (Moving == MoveTypes::BossMechanic) Functions::StopMovement();
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
        Functions::StopMovement();
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
    if (localPlayer == nullptr) return;
    const auto currentMap = static_cast<unsigned int>(mapID);
    const auto currentZone = static_cast<unsigned int>(localPlayer->zoneID);
    std::vector<DangerZone> zones;
    bool hasApplicableRule = false;
    for (const auto& rule : DangerRules) {
        if (rule.EntryId != 0 && (rule.MapId == 0 || rule.MapId == currentMap) &&
            (rule.ZoneId == 0 || rule.ZoneId == currentZone)) {
            hasApplicableRule = true;
            break;
        }
    }
    if (!hasApplicableRule) {
        Navigation::SetNpcDangerZones(currentMap, zones);
        return;
    }
    zones.reserve(DangerRules.size());
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
