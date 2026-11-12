#define NOMINMAX
#include "Functions.h"

#include "../MemoryManager.h"
#include "../Game.h"
#include "../Navigation.h"
#include "../FactionTemplate.h"
#include "../DangerZoneGeometry.h"
#include "../WaterMap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <vector>

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float ANGLE_STEP = PI / 8.0f;
    constexpr float MIN_MOVE_DISTANCE = 2.0f;
    constexpr float SWIM_BOTTOM_CLEARANCE = 0.60f;
    constexpr float SWIM_SURFACE_CLEARANCE = 0.35f;
    constexpr std::array<int, 13> MOVE_OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

    Position ClosestPointOnSegment(const Position& point, const Position& start, const Position& end) {
        const float dx = end.X - start.X, dy = end.Y - start.Y, dz = end.Z - start.Z;
        const float lengthSq = dx * dx + dy * dy + dz * dz;
        if (lengthSq < 0.000001f) return start;
        const float t = std::clamp(((point.X - start.X) * dx +
            (point.Y - start.Y) * dy + (point.Z - start.Z) * dz) / lengthSq, 0.0f, 1.0f);
        return Position(start.X + t * dx, start.Y + t * dy, start.Z + t * dz);
    }

    bool IsPathSafeFromEnemies(const Position& start, const Position& end, unsigned long long ignoredGuid = 0) {
        for (auto& enemy : ListUnits) {
            if (enemy.Guid == ignoredGuid || !enemy.attackable || enemy.isdead || FactionTemplate.isNeutral(enemy.factionTemplateID) ||
                enemy.unitReaction >= Neutral || enemy.isInCombatOrEncounter() ||
                (enemy.flags & UNIT_FLAG_PLAYER_CONTROLLED) || enemy.creatureType == Totem || enemy.createdBy != 0 ||
                ((enemy.movement_flags & MOVEFLAG_SWIMMING) && !(localPlayer->movement_flags & MOVEFLAG_SWIMMING))) continue;
            const float radius = 21.0f + static_cast<float>(enemy.level - localPlayer->level);
            if (radius <= 0.0f) continue;
            const Position nearest = ClosestPointOnSegment(enemy.position, start, end);
            const float current = enemy.position.DistanceTo(start);
            const float closest = enemy.position.DistanceTo(nearest);
            if (((localPlayer->movement_flags & MOVEFLAG_SWIMMING) ||
                std::abs(enemy.position.Z - nearest.Z) < 20.0f) && closest < radius &&
                (current >= radius || closest + 0.001f < current) && !Functions::Intersect(enemy.position, nearest)) return false;
        }
        return true;
    }

    struct WaterCorridor {
        bool crosses = false;
        bool entryNear = false;
        float groundHorizon = 16.0f;
    };

    WaterCorridor InspectWaterCorridor(const Position& start, const Position& goal,
        float horizon = 16.0f) {
        WaterCorridor result;
        const float length = start.DistanceTo2D(goal);
        const float checked = (std::min)(length, horizon);
        if (checked < 0.01f) return result;
        const int samples = static_cast<int>(std::ceil(checked / 2.0f));
        for (int i = 1; i <= samples; ++i) {
            const float distance = (std::min)(checked, i * 2.0f);
            const float t = distance / length;
            const Position point(start.X + (goal.X - start.X) * t,
                start.Y + (goal.Y - start.Y) * t,
                start.Z + (goal.Z - start.Z) * t);
            const auto water = WaterMap::At(mapID, point.X, point.Y, point.Z);
            if (!water.CanSwim()) continue;
            if (water.RelativeTo(point.Z) == WaterMap::Level::Above &&
                start.Z > water.surface + 1.5f && goal.Z > water.surface + 1.5f) {
                const Position ground = Functions::ProjectPos(point);
                if (ground.DistanceTo(point) <= 1.25f &&
                    ground.Z > water.surface + 0.75f) continue;
            }
            result.crosses = true;
            result.entryNear = distance <= 4.0f;
            result.groundHorizon = (std::max)(2.5f, distance - 2.0f);
            break;
        }
        return result;
    }

    bool IsGroundSafe8Points(const Position& center, float radius, int sampleCount = 8) {
        constexpr int POINT_COUNT = 8;
        constexpr float FULL_CIRCLE = 2.0f * PI;
        constexpr float POINT_ANGLE_STEP = FULL_CIRCLE / static_cast<float>(POINT_COUNT);
        static const std::array<std::array<float, 2>, POINT_COUNT> directions = [=] {
            std::array<std::array<float, 2>, POINT_COUNT> values{};
            for (int i = 0; i < POINT_COUNT; ++i) {
                const float angle = static_cast<float>(i) * POINT_ANGLE_STEP;
                values[i] = { std::cos(angle), std::sin(angle) };
            }
            return values;
            }();
        for (int i = 0; i < POINT_COUNT; i += sampleCount == 4 ? 2 : 1) {
            Position probe(
                center.X + directions[i][0] * radius,
                center.Y + directions[i][1] * radius,
                center.Z
            );
            Position projected = Functions::ProjectPos(probe);
            if (projected.DistanceTo(probe) >= 2.0f || Functions::Intersect(projected, center, 1.5f)) return false;
        }
        return true;
    }

    bool ProjectSafeGroundStep(const Position& from, const Position& wanted, Position& result,
        float groundCheckRadius, const Navigation::AvoidanceSnapshot& zones,
        bool checkFootprint = true, bool fullFootprint = true) {
        if (!Navigation::IsSegmentSafe(zones, from, wanted)) return false;
        const auto water = WaterMap::At(mapID, wanted.X, wanted.Y, wanted.Z);
        if (water.kind == WaterMap::Kind::Hazard) return false;
        result = Functions::ProjectPos(wanted);
        if (water.CanSwim() && result.Z <= water.surface + 0.75f) return false;
        return result.DistanceTo(wanted) < 2.0f &&
            std::abs(result.Z - from.Z) <= 1.0f &&
            !Functions::Intersect(from, result, 1.5f) &&
            Navigation::IsSegmentSafe(zones, from, result) &&
            (!checkFootprint || IsGroundSafe8Points(result, groundCheckRadius, fullFootprint ? 8 : 4));
    }

    bool HasSafeGroundContinuation(const Position& from, const Position& goal,
        const Navigation::AvoidanceSnapshot& zones) {
        const float remaining = from.DistanceTo2D(goal);
        if (remaining <= 2.0f) return true;
        const float t = (std::min)(3.0f, remaining) / remaining;
        const Position ahead(from.X + (goal.X - from.X) * t,
            from.Y + (goal.Y - from.Y) * t,
            from.Z + (goal.Z - from.Z) * t);
        Position projected;
        return ProjectSafeGroundStep(from, ahead, projected, 1.25f, zones, false);
    }

    bool HasDirectGroundRoute(const Position& from, const Position& goal,
        const Navigation::AvoidanceSnapshot& zones,
        bool checkEnemyClose = false) {
        constexpr float STEP = 2.0f;
        constexpr int MAX_STEPS = 15;
        const float distance = from.DistanceTo2D(goal);
        if (distance < 0.01f || distance > STEP * MAX_STEPS ||
            !Navigation::IsSegmentSafe(zones, from, goal)) return false;
        const int steps = static_cast<int>(std::ceil(distance / STEP));
        Position previous = from;
        for (int i = 1; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const Position wanted(from.X + (goal.X - from.X) * t,
                from.Y + (goal.Y - from.Y) * t,
                from.Z + (goal.Z - from.Z) * t);
            Position projected;
            const bool checkFootprint = (i % 3 == 0 || i == steps);
            if (!ProjectSafeGroundStep(previous, wanted, projected, 1.25f, zones, checkFootprint, i == steps)) return false;
            previous = projected;
        }
        return !checkEnemyClose || IsPathSafeFromEnemies(from, goal);
    }

    bool HasValidatedPivot(const Position& start, const Position& pivot, const Position& goal,
        const Navigation::AvoidanceSnapshot& zones) {
        if (!HasDirectGroundRoute(start, pivot, zones)) return false;
        const float distance = start.DistanceTo2D(pivot);
        if (distance < MIN_MOVE_DISTANCE) return false;
        const float t = (std::max)(0.0f, (distance - 1.5f) / distance);
        const Position approach(start.X + (pivot.X - start.X) * t,
            start.Y + (pivot.Y - start.Y) * t,
            start.Z + (pivot.Z - start.Z) * t);
        const Position groundApproach = Functions::ProjectPos(approach);
        if (groundApproach.DistanceTo(approach) >= 1.0f) return false;
        return HasDirectGroundRoute(groundApproach, goal, zones);
    }

    struct FailedHeading {
        Position Origin;
        Position Destination;
        int MapId;
        std::chrono::steady_clock::time_point Since;
    };
    std::vector<FailedHeading> FailedHeadings;

    Position SwimDestination(const Position& target, const WaterMap::Sample& water) {
        if (!water.CanSwim() || target.Z > water.surface + 0.75f) return target;
        return Position(target.X, target.Y,
            std::clamp(target.Z, water.ground + SWIM_BOTTOM_CLEARANCE,
                water.surface - SWIM_SURFACE_CLEARANCE));
    }

    void CompleteMovement(bool started, MoveTypes type) {
        if (started) Moving = type;
        else Functions::StopMovement();
    }

    bool IsRecentlyFailedHeading(const Position& start, const Position& candidate) {
        const auto now = std::chrono::steady_clock::now();
        FailedHeadings.erase(std::remove_if(FailedHeadings.begin(), FailedHeadings.end(),
            [now](const FailedHeading& failure) {
                return now - failure.Since > std::chrono::milliseconds(1800);
            }), FailedHeadings.end());
        const float dx = candidate.X - start.X, dy = candidate.Y - start.Y;
        const float length = std::hypot(dx, dy);
        if (length < MIN_MOVE_DISTANCE) return false;
        for (const auto& failure : FailedHeadings) {
            if (failure.MapId != mapID || start.DistanceTo2D(failure.Origin) > 2.5f) continue;
            const float fx = failure.Destination.X - failure.Origin.X;
            const float fy = failure.Destination.Y - failure.Origin.Y;
            const float flength = std::hypot(fx, fy);
            if (flength > MIN_MOVE_DISTANCE && (dx * fx + dy * fy) / (length * flength) > 0.85f) return true;
        }
        return false;
    }

    bool TryServerWaypoint(const Position& goal, bool avoidEnemies,
        unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const Position waypoint = Navigation::CalculatePath(mapID, start, goal);
        return start.DistanceTo(waypoint) >= MIN_MOVE_DISTANCE &&
            Navigation::IsSegmentSafe(mapID, start, waypoint) &&
            (!avoidEnemies || IsPathSafeFromEnemies(start, waypoint, ignoredGuid)) &&
            Functions::StartMovement(waypoint, false, true);
    }

    struct LateralRecoveryState {
        bool Active = false;
        int MapId = -1;
        MoveTypes Type = MoveTypes::NotMoving;
        int LastSide = 0;
        Position Destination{};
        Position LastProgressPosition{};
        std::chrono::steady_clock::time_point LastProgressAt{};
        std::chrono::steady_clock::time_point Expires{};
    };
    LateralRecoveryState LateralRecovery;

    bool TryLocalRecoveryStep(const Position& destination, bool checkEnemyClose,
        bool allowPartial = false, unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
        if (Navigation::IsPositionDangerous(zones, destination) ||
            start.DistanceTo2D(destination) < MIN_MOVE_DISTANCE) return false;
        const float base = std::atan2(destination.Y - start.Y, destination.X - start.X);
        for (float stride : {3.0f, 5.0f, 8.0f}) {
            std::vector<std::pair<float, Position>> candidates;
            candidates.reserve(MOVE_OFFSETS.size());
            for (int off : MOVE_OFFSETS) {
                const float dir = base + off * ANGLE_STEP;
                const Position wanted(start.X + std::cos(dir) * stride,
                    start.Y + std::sin(dir) * stride, start.Z);
                Position next;
                if (IsRecentlyFailedHeading(start, wanted) ||
                    !ProjectSafeGroundStep(start, wanted, next, 1.25f, zones, false) ||
                    !Navigation::IsSegmentSafe(zones, start, next) ||
                    IsRecentlyFailedHeading(start, next) ||
                    start.DistanceTo(next) < MIN_MOVE_DISTANCE ||
                    (checkEnemyClose && !IsPathSafeFromEnemies(start, next, ignoredGuid))) continue;
                candidates.emplace_back(next.DistanceTo2D(destination) +
                    std::abs(off) * 0.08f + stride * 0.08f, next);
            }
            std::sort(candidates.begin(), candidates.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            for (const auto& entry : candidates) {
                const Position& candidate = entry.second;
                const bool partial = allowPartial && start.DistanceTo2D(destination) > 30.0f &&
                    candidate.DistanceTo2D(destination) + MIN_MOVE_DISTANCE < start.DistanceTo2D(destination);
                if (partial ? (!HasSafeGroundContinuation(candidate, destination, zones) ||
                    !HasDirectGroundRoute(start, candidate, zones)) :
                    !HasValidatedPivot(start, candidate, destination, zones)) continue;
                if (Functions::StartMovement(candidate)) return true;
            }
        }
        return false;
    }

    bool TryGroundMove(const Position& goal, bool avoidEnemies, bool findLineOfSight,
        bool allowPartial = false, float partialHorizon = 16.0f,
        unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
        if (!findLineOfSight && Navigation::IsPositionDangerous(zones, goal)) return false;
        const float distance = start.DistanceTo2D(goal);
        if (!allowPartial && !findLineOfSight && start.DistanceTo(goal) > 40.0f) return false;
        if (!findLineOfSight) {
            if (distance >= MIN_MOVE_DISTANCE && distance <= (allowPartial ? partialHorizon : 30.0f) &&
                HasDirectGroundRoute(start, goal, zones) &&
                (!avoidEnemies || IsPathSafeFromEnemies(start, goal, ignoredGuid)))
                return Functions::StartMovement(goal);
            if (allowPartial && distance > partialHorizon) {
                const float t = partialHorizon / distance;
                const Position horizon(start.X + (goal.X - start.X) * t,
                    start.Y + (goal.Y - start.Y) * t,
                    start.Z + (goal.Z - start.Z) * t);
                if (HasDirectGroundRoute(start, horizon, zones) &&
                    !IsRecentlyFailedHeading(start, horizon) &&
                    (!avoidEnemies || IsPathSafeFromEnemies(start, horizon, ignoredGuid)) &&
                    Functions::StartMovement(horizon)) return true;
            }
        }
        const float step = findLineOfSight ? 2.5f : 2.0f;
        const int maxSteps = findLineOfSight ? 12 : 8;
        const float base = std::atan2(goal.Y - start.Y, goal.X - start.X);
        std::vector<std::pair<float, Position>> partialCandidates;
        partialCandidates.reserve(MOVE_OFFSETS.size());
        const size_t directionCount = allowPartial && !findLineOfSight ? 9 : MOVE_OFFSETS.size();
        for (size_t directionIndex = 0; directionIndex < directionCount; ++directionIndex) {
            const int offset = MOVE_OFFSETS[directionIndex];
            const float direction = base + offset * ANGLE_STEP;
            const float stepX = std::cos(direction) * step;
            const float stepY = std::sin(direction) * step;
            Position previous = start;
            Position furthest{};
            bool havePartial = false;
            for (int i = 0; i < maxSteps; ++i) {
                const Position wanted(previous.X + stepX, previous.Y + stepY, previous.Z);
                Position candidate;
                if (!ProjectSafeGroundStep(previous, wanted, candidate, 1.25f, zones, false)) break;
                if (findLineOfSight) {
                    if (!Functions::Intersect(candidate, goal)) {
                        if (start.DistanceTo(candidate) >= MIN_MOVE_DISTANCE &&
                            HasDirectGroundRoute(start, candidate, zones) &&
                            (!avoidEnemies || IsPathSafeFromEnemies(start, candidate, ignoredGuid)) &&
                            Functions::StartMovement(candidate)) return true;
                        break;
                    }
                } else if (i >= 2 && i % 3 == 2 &&
                           start.DistanceTo2D(candidate) >= MIN_MOVE_DISTANCE &&
                           !IsRecentlyFailedHeading(start, candidate) &&
                           candidate.DistanceTo2D(goal) <= 30.0f &&
                           HasValidatedPivot(start, candidate, goal, zones) &&
                           (!avoidEnemies || IsPathSafeFromEnemies(start, candidate, ignoredGuid)) &&
                           Functions::StartMovement(candidate)) {
                    return true;
                }
                if (allowPartial && !findLineOfSight && i >= 2 &&
                    start.DistanceTo2D(goal) - candidate.DistanceTo2D(goal) > MIN_MOVE_DISTANCE) {
                    furthest = candidate;
                    havePartial = true;
                }
                previous = candidate;
            }
            if (havePartial && !IsRecentlyFailedHeading(start, furthest)) {
                const float score = furthest.DistanceTo2D(goal) + 0.1f * std::abs(offset);
                partialCandidates.emplace_back(score, furthest);
            }
        }
        if (allowPartial && !findLineOfSight) {
            std::sort(partialCandidates.begin(), partialCandidates.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            int checked = 0;
            for (const auto& candidate : partialCandidates) {
                if (++checked > 4) break;
                if (HasSafeGroundContinuation(candidate.second, goal, zones) &&
                    HasDirectGroundRoute(start, candidate.second, zones) &&
                    (!avoidEnemies || IsPathSafeFromEnemies(start, candidate.second, ignoredGuid)) &&
                    Functions::StartMovement(candidate.second)) return true;
            }
        }
        return false;
    }

    bool TrySwimMove(const Position& goal, bool avoidEnemies, bool findLineOfSight,
        unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const auto water = WaterMap::At(mapID, goal.X, goal.Y, goal.Z);
        if (water.kind == WaterMap::Kind::Hazard) return false;
        Position destination = SwimDestination(goal, water);
        if (start.DistanceTo(destination) <= 0.75f) return true;
        if (std::isfinite(localPlayer->facing) && start.DistanceTo2D(destination) < 0.75f &&
            std::abs(start.Z - destination.Z) > 1.5f) {
            const Position angled(destination.X + std::cos(localPlayer->facing) * 2.0f,
                destination.Y + std::sin(localPlayer->facing) * 2.0f, destination.Z);
            if (Navigation::IsSegmentSafe(mapID, start, angled) &&
                !Functions::Intersect(start, angled, 0.0f) &&
                WaterMap::At(mapID, angled.X, angled.Y, angled.Z).kind != WaterMap::Kind::Hazard)
                destination = angled;
        }
        if (!Navigation::IsSegmentSafe(mapID, start, destination) ||
            Functions::Intersect(start, destination, 0.0f) ||
            (avoidEnemies && !IsPathSafeFromEnemies(start, destination, ignoredGuid)) ||
            (findLineOfSight && Functions::Intersect(destination, goal))) return false;
        return Functions::StartMovement(destination);
    }

    bool TryWaterEntryMove(const Position& goal, bool avoidEnemies,
        unsigned long long ignoredGuid = 0, const Position* retreatFrom = nullptr) {
        const Position start = localPlayer->position;
        return start.DistanceTo(goal) >= MIN_MOVE_DISTANCE &&
            (!retreatFrom || goal.DistanceTo(*retreatFrom) >
                start.DistanceTo(*retreatFrom) + 0.25f) &&
            TrySwimMove(goal, avoidEnemies, false, ignoredGuid);
    }

    bool ChooseMeleeApproach(Position& target_pos, bool meleeTarget, bool checkEnemyClose,
        const Navigation::AvoidanceSnapshot& zones) {
        if (Navigation::IsPositionDangerous(zones, target_pos) && meleeTarget) {
            const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
            const float range = localPlayer->combatReach + targetUnit->combatReach + 1.0f;
            bool found = false;
            Position best;
            const float base = std::atan2(localPlayer->position.Y - target_pos.Y,
                localPlayer->position.X - target_pos.X);
            std::array<std::pair<float, Position>, 24> positions{};
            for (int i = 0; i < 24; ++i) {
                const float angle = base + i * (2.0f * PI / 24.0f);
                const Position desired(target_pos.X + std::cos(angle) * range,
                    target_pos.Y + std::sin(angle) * range, target_pos.Z);
                positions[i] = { swimming ? localPlayer->position.DistanceTo(desired) :
                    localPlayer->position.DistanceTo2D(desired), desired };
            }
            std::sort(positions.begin(), positions.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            int checked = 0;
            for (const auto& [score, desired] : positions) {
                if (++checked > 10) break;
                if (Navigation::IsPositionDangerous(zones, desired)) continue;
                Position candidate = desired;
                if (swimming) {
                    const auto water = WaterMap::At(mapID, desired.X, desired.Y, desired.Z);
                    if (water.Known() && !water.CanSwim()) continue;
                    if (water.CanSwim()) candidate.Z = std::clamp(target_pos.Z, water.ground + 0.6f,
                        (std::max)(water.ground + 0.6f, water.surface - 0.35f));
                    if (Functions::Intersect(localPlayer->position, candidate, 1.25f)) continue;
                } else {
                    candidate = Functions::ProjectPos(desired);
                    if (candidate.DistanceTo(desired) >= 2.0f ||
                        !IsGroundSafe8Points(candidate, 1.0f)) continue;
                }
                if (Navigation::IsPositionDangerous(zones, candidate) ||
                    (checkEnemyClose && !IsPathSafeFromEnemies(
                        localPlayer->position, candidate, targetUnit->Guid)) ||
                    Functions::Intersect(candidate, target_pos)) continue;
                best = candidate;
                found = true;
                break;
            }
            if (!found) return false;
            target_pos = best;
        }
        return true;
    }

    bool TryCombatDetour(const Position& target_pos, bool checkEnemyClose,
        const Navigation::AvoidanceSnapshot& zones) {
        const Position start = localPlayer->position;
        const DangerZone* blocking = nullptr;
        float nearestT = 2.0f;
        const float dx = target_pos.X - start.X;
        const float dy = target_pos.Y - start.Y;
        const float lenSq = dx * dx + dy * dy;
        for (const auto& zone : *zones) {
            const float radius = zone.Radius + DangerZoneGeometry::SafetyMargin;
            if (!DangerZoneGeometry::BlocksSegment(start, target_pos, zone.Center, radius) ||
                DangerZoneGeometry::Contains(start, zone.Center, radius)) continue;
            const float t = lenSq > 0.0f ? std::clamp(((zone.Center.X - start.X) * dx +
                (zone.Center.Y - start.Y) * dy) / lenSq, 0.0f, 1.0f) : 0.0f;
            if (t < nearestT) {
                nearestT = t;
                blocking = &zone;
            }
        }
        if (blocking) {
            const float startAngle = std::atan2(start.Y - blocking->Center.Y,
                start.X - blocking->Center.X);
            const float goalAngle = std::atan2(target_pos.Y - blocking->Center.Y,
                target_pos.X - blocking->Center.X);
            const float angleDifference = std::atan2(std::sin(goalAngle - startAngle),
                std::cos(goalAngle - startAngle));
            const int side = std::abs(angleDifference) < 0.05f ?
                (localPlayer->Guid % 2ULL ? 1 : -1) : (angleDifference > 0.0f ? 1 : -1);
            const float radius = blocking->Radius + DangerZoneGeometry::SafetyMargin + 2.5f;
            for (int direction : {side, -side}) {
                for (float angle : {0.25f, 0.45f, 0.65f, 0.90f}) {
                    const float stepAngle = startAngle + direction * angle;
                    const Position desired(blocking->Center.X + radius * std::cos(stepAngle),
                        blocking->Center.Y + radius * std::sin(stepAngle), start.Z);
                    Position candidate = Functions::ProjectPos(desired);
                    if (candidate.DistanceTo(desired) >= 2.0f) continue;
                    const float length = start.DistanceTo2D(candidate);
                    if (length > 28.0f) {
                        const float t = 26.0f / length;
                        const Position closer(start.X + (candidate.X - start.X) * t,
                            start.Y + (candidate.Y - start.Y) * t,
                            start.Z + (candidate.Z - start.Z) * t);
                        candidate = Functions::ProjectPos(closer);
                        if (candidate.DistanceTo(closer) >= 2.0f) continue;
                    }
                    if (start.DistanceTo2D(candidate) < MIN_MOVE_DISTANCE ||
                        Navigation::IsPositionDangerous(zones, candidate) ||
                        IsRecentlyFailedHeading(start, candidate) ||
                        !HasDirectGroundRoute(start, candidate, zones) ||
                        (checkEnemyClose && !IsPathSafeFromEnemies(start, candidate,
                            targetUnit ? targetUnit->Guid : 0))) continue;
                    if (Functions::StartMovement(candidate)) return true;
                }
            }
        }
        return false;
    }
}

bool Functions::StartMovement(const Position& destination, bool emergency,
    bool navigationWaypoint, unsigned long long guid) {
    if (!localPlayer || !std::isfinite(destination.X) || !std::isfinite(destination.Y) ||
        !std::isfinite(destination.Z)) return false;
    const Position start = localPlayer->position;
    if (!emergency && !(localPlayer->movement_flags & MOVEFLAG_SWIMMING) &&
        IsRecentlyFailedHeading(start, destination)) {
        if (!navigationWaypoint) return false;
        for (const auto& failed : FailedHeadings)
            if (failed.MapId == mapID && start.DistanceTo2D(failed.Origin) <= 5.0f &&
                destination.DistanceTo2D(failed.Destination) <= 2.0f) return false;
    }
    if (!Navigation::IsSegmentSafe(mapID, start, destination)) return false;
    localPlayer->ClickToMove(Move, guid ? guid : localPlayer->Guid, destination);
    Navigation::RecordMovement(start, destination);
    return true;
}

void Functions::StopMovement() {
    if (localPlayer) {
        if (localPlayer->movement_flags & MOVEFLAG_BACKWARD)
            ThreadSynchronizer::releaseKey(0x28);
        else if (localPlayer->isMoving || (localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
            ThreadSynchronizer::pressKey(0x28);
            ThreadSynchronizer::releaseKey(0x28);
        }
    }
    LateralRecovery.Active = false;
    Navigation::ClearMovement();
    Moving = MoveTypes::NotMoving;
}

bool Functions::AscendForBreath() {
    if (!localPlayer || !(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) return false;
    const Position start = localPlayer->position;
    const auto water = WaterMap::At(mapID, start.X, start.Y, start.Z);
    if (!water.CanSwim()) return false;
    const Position surface(
        start.X + std::cos(localPlayer->facing) * 2.0f,
        start.Y + std::sin(localPlayer->facing) * 2.0f,
        water.surface
    );
    if (start.Z >= surface.Z) return true;
    if (!Navigation::IsSegmentSafe(mapID, start, surface) ||
        Functions::Intersect(start, surface, 0.0f)) return false;
    return Functions::StartMovement(surface, true);
}

bool Functions::EscapeDangerZone() {
    const Position start = localPlayer->position;
    if (!Navigation::IsPositionDangerous(mapID, start)) return false;

    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const DangerZone* nearest = nullptr;
    float greatestPenetration = -1.0e9f;
    for (const auto& zone : *zones) {
        const float penetration = zone.Radius + 1.5f - start.DistanceTo2D(zone.Center);
        if (std::abs(start.Z - zone.Center.Z) <= 6.0f && penetration > 0.0f &&
            penetration > greatestPenetration) {
            greatestPenetration = penetration;
            nearest = &zone;
        }
    }
    if (!nearest) return false;

    const float base = std::atan2(start.Y - nearest->Center.Y, start.X - nearest->Center.X);
    Position best;
    bool found = false;
    float bestScore = -1.0e9f;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    for (const float stride : {3.0f, 6.0f, 10.0f, 15.0f}) {
        for (int off : MOVE_OFFSETS) {
            const float dir = base + off * ANGLE_STEP;
            Position wanted(start.X + std::cos(dir) * stride,
                start.Y + std::sin(dir) * stride, start.Z);
            Position candidate;
            if (swimming) {
                candidate = wanted;
                if (Functions::Intersect(start, candidate, 1.25f)) continue;
            }
            else if (!ProjectSafeGroundStep(start, wanted, candidate, 3.0f, zones)) continue;
            if (!Navigation::IsSegmentSafe(zones, start, candidate)) continue;
            if (start.DistanceTo(candidate) < MIN_MOVE_DISTANCE) continue;
            float clearance = 1.0e9f;
            for (const auto& zone : *zones) {
                if (std::abs(candidate.Z - zone.Center.Z) <= 6.0f) {
                    clearance = (std::min)(clearance,
                        candidate.DistanceTo2D(zone.Center) - zone.Radius - 1.5f);
                }
            }
            const float score = clearance - stride * 0.04f - std::abs(off) * 0.05f -
                (Functions::enemyClose(candidate, true) ? 5.0f : 0.0f);
            if (score > bestScore) {
                best = candidate;
                bestScore = score;
                found = true;
            }
        }
    }
    if (found && Functions::StartMovement(best, true)) {
        Moving = MoveTypes::BossMechanic;
    } else Functions::StopMovement();
    return true;
}

bool Functions::UpdateLateralRecovery() {
    if (!LateralRecovery.Active) return false;
    if (localPlayer == nullptr || LateralRecovery.MapId != mapID ||
        Moving != LateralRecovery.Type) {
        LateralRecovery.Active = false;
        return false;
    }
    if (Navigation::IsCurrentMovementUnsafe(mapID, localPlayer->position)) {
        LateralRecovery.Active = false;
        Functions::StopMovement();
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    if (localPlayer->position.DistanceTo2D(LateralRecovery.LastProgressPosition) >= 0.3f) {
        LateralRecovery.LastProgressPosition = localPlayer->position;
        LateralRecovery.LastProgressAt = now;
    }
    const bool arrived = localPlayer->position.DistanceTo2D(LateralRecovery.Destination) <= 0.85f;
    const bool expired = now >= LateralRecovery.Expires;
    const bool interrupted = now - LateralRecovery.LastProgressAt >= std::chrono::milliseconds(650);
    if (arrived || expired || interrupted) {
        Functions::StopMovement();
        return false;
    }
    return true;
}

void Functions::HandleMovementStall() {
    if (LateralRecovery.Active || localPlayer == nullptr) return;
    Position failedDestination;
    const MoveTypes previousType = Moving;
    const bool hasFailedWaypoint = Navigation::GetCurrentMovementDestination(failedDestination) &&
        localPlayer->position.DistanceTo2D(failedDestination) >= MIN_MOVE_DISTANCE;
    if (hasFailedWaypoint) {
        FailedHeadings.push_back({ localPlayer->position, failedDestination, mapID,
                                  std::chrono::steady_clock::now() });
        if (FailedHeadings.size() > 8) FailedHeadings.erase(FailedHeadings.begin());
    }
    Functions::StopMovement();
    const unsigned int flags = localPlayer->movement_flags;
    if (!hasFailedWaypoint ||
        previousType == MoveTypes::NotMoving || previousType == MoveTypes::BossMechanic ||
        (flags & (MOVEFLAG_ROOT | MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR |
                  MOVEFLAG_SWIMMING | MOVEFLAG_FLYING | MOVEFLAG_FLYING2)) ||
        !std::isfinite(localPlayer->facing)) return;

    constexpr float SIDE_DISTANCE = 3.5f;
    constexpr int SAMPLE_COUNT = 5;
    constexpr float OUTER_EDGE_MARGIN = 0.35f;
    constexpr float MAX_HEIGHT_FROM_START = 1.5f;
    constexpr float MAX_STEP_HEIGHT = 0.65f;

    const Position start = localPlayer->position;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const float leftX = -std::sin(localPlayer->facing);
    const float leftY = std::cos(localPlayer->facing);
    const int preferred = LateralRecovery.LastSide == 1 ? -1 : 1;
    for (const float distanceToSide : {SIDE_DISTANCE, 2.5f}) {
        for (int side : {preferred, -preferred}) {
            const float dirX = leftX * side, dirY = leftY * side;
            Position previous = start;
            Position candidate;
            bool safe = true;
            for (int sample = 1; sample <= SAMPLE_COUNT; ++sample) {
                const float distance = distanceToSide * sample / SAMPLE_COUNT;
                const Position point(start.X + dirX * distance, start.Y + dirY * distance, start.Z);
                const Position ground = Functions::ProjectPos(point);
                if (!std::isfinite(ground.Z) ||
                    std::abs(ground.Z - start.Z) > MAX_HEIGHT_FROM_START ||
                    std::abs(ground.Z - previous.Z) > MAX_STEP_HEIGHT ||
                    !Navigation::IsSegmentSafe(zones, previous, ground) ||
                    Functions::Intersect(previous, ground, 1.5f)) {
                    safe = false;
                    break;
                }
                if (sample == 3 || sample == SAMPLE_COUNT) {
                    const Position outerEdge(point.X + dirX * OUTER_EDGE_MARGIN,
                        point.Y + dirY * OUTER_EDGE_MARGIN, start.Z);
                    const Position outerGround = Functions::ProjectPos(outerEdge);
                    if (!std::isfinite(outerGround.Z) ||
                        std::abs(outerGround.Z - ground.Z) > MAX_STEP_HEIGHT ||
                        Navigation::IsPositionDangerous(zones, outerGround) ||
                        !Navigation::IsSegmentSafe(zones, ground, outerGround)) {
                        safe = false;
                    }
                }
                if (!safe) break;
                candidate = ground;
                previous = ground;
            }
            if (!safe || IsRecentlyFailedHeading(start, candidate) ||
                !Functions::StartMovement(candidate)) continue;
            LateralRecovery.Active = true;
            LateralRecovery.MapId = mapID;
            LateralRecovery.Type = previousType;
            LateralRecovery.LastSide = side;
            LateralRecovery.Destination = candidate;
            LateralRecovery.LastProgressPosition = start;
            LateralRecovery.LastProgressAt = std::chrono::steady_clock::now();
            LateralRecovery.Expires = LateralRecovery.LastProgressAt + std::chrono::milliseconds(1350);
            Moving = previousType;
            return;
        }
    }
}

bool Functions::StepBack(WoWUnit* target, MoveTypes moveType, float dist_away) {
    if (!target || !localPlayer) return false;
    const Position start = localPlayer->position;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const float needed = dist_away + localPlayer->combatReach + target->combatReach;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    const float currentDistance = swimming ? start.DistanceTo(target->position) : start.DistanceTo2D(target->position);
    if (currentDistance >= needed) return true;
    const float angle = start.DistanceTo2D(target->position) > 0.01f ?
        std::atan2(start.Y - target->position.Y, start.X - target->position.X) :
        localPlayer->facing + PI;
    struct Retreat { float score; Position point; };
    std::vector<Retreat> candidates;
    candidates.reserve(52);
    for (float stride : { 6.0f, 12.0f, 18.0f, 24.0f }) {
        for (int offset : MOVE_OFFSETS) {
            const float dir = angle + offset * ANGLE_STEP;
            const Position wanted(start.X + std::cos(dir) * stride,
                start.Y + std::sin(dir) * stride, start.Z);
            const float distance = swimming ? wanted.DistanceTo(target->position) : wanted.DistanceTo2D(target->position);
            if (distance < needed || distance <= currentDistance ||
                Navigation::IsPositionDangerous(zones, wanted) ||
                !Navigation::IsSegmentSafe(zones, start, wanted) ||
                (!swimming && IsRecentlyFailedHeading(start, wanted))) continue;
            const float score = stride + std::abs(offset) * 0.35f;
            candidates.push_back({score, wanted});
        }
    }
    std::sort(candidates.begin(), candidates.end(),
        [](const Retreat& a, const Retreat& b) { return a.score < b.score; });
    int attempts = 0;
    for (const auto& candidate : candidates) {
        if (++attempts > 10) break;
        Position chosen = candidate.point;
        if (swimming) {
            const auto water = WaterMap::At(mapID, chosen.X, chosen.Y, chosen.Z);
            if (water.kind == WaterMap::Kind::Hazard) continue;
            if (water.CanSwim()) {
                chosen.Z = std::clamp(chosen.Z, water.ground + 0.6f,
                    (std::max)(water.ground + 0.6f, water.surface - 0.35f));
            } else if (water.kind == WaterMap::Kind::Dry ||
                       water.kind == WaterMap::Kind::Shallow) {
                const Position projected = Functions::ProjectPos(chosen);
                if (std::abs(projected.Z - chosen.Z) > 2.0f) continue;
                chosen = projected;
            }
            if (Functions::Intersect(start, chosen, 1.25f)) continue;
        } else {
            Position projected = Functions::ProjectPos(chosen);
            if (projected.DistanceTo(chosen) >= 2.0f ||
                !HasDirectGroundRoute(start, projected, zones)) continue;
            chosen = projected;
        }
        if ((!swimming || chosen.DistanceTo(target->position) >= needed) &&
            IsPathSafeFromEnemies(start, chosen, target->Guid) &&
            !Functions::Intersect(chosen, target->position) &&
            Functions::StartMovement(chosen)) {
            Moving = moveType;
            return true;
        }
    }
    if (!swimming) {
        int checked = 0;
        for (const auto& candidate : candidates) {
            if (++checked > 10) break;
            const auto water = InspectWaterCorridor(start, candidate.point);
            if (!water.crosses || !water.entryNear) continue;
            if (TryWaterEntryMove(candidate.point, true, target->Guid,
                &target->position)) {
                Moving = moveType;
                return true;
            }
        }
    }
    Functions::StopMovement();
    return false;
}

void Functions::FollowMultibox(int placement) {
    if (Leader == NULL) return;

    ThreadSynchronizer::RunOnMainThread([placement]() {
        if (!Leader || !localPlayer) return;
        float range = 2.0f;
        float offset = 0.30f;
        if (placement == 2) offset = -0.30f;
        else if (placement == 3) {
            range = 4.0f;
            offset = localPlayer->isMounted ? 0.45f : 0.15f;
        }
        else if (placement == 4) {
            range = 4.0f;
            offset = localPlayer->isMounted ? -0.45f : -0.15f;
        }
        const float angle = Leader->facing + PI + offset;
        const Position target_pos(Leader->position.X + std::cos(angle) * range,
            Leader->position.Y + std::sin(angle) * range, Leader->position.Z);
        const bool targetSwim = (Leader->movement_flags & MOVEFLAG_SWIMMING) != 0;
        const float followDistance = localPlayer->position.DistanceTo(target_pos);
        if (followDistance <= 2.5f) {
            if (Moving == MoveTypes::Follow) Functions::StopMovement();
            return;
        }
        if (Moving != MoveTypes::Follow && followDistance < 4.0f) return;
        Functions::MoveTo(target_pos, MoveTypes::Follow, true, targetSwim);
    });
}

void Functions::MoveToFast(Position target_pos, MoveTypes moveType) {
    if (!localPlayer || (LateralRecovery.Active && moveType != MoveTypes::BossMechanic)) return;
    LateralRecovery.Active = false;
    const Position start = localPlayer->position;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    if (start.DistanceTo(target_pos) < (swimming ? 0.75f : MIN_MOVE_DISTANCE)) {
        if (Moving == moveType) StopMovement();
        return;
    }
    const auto water = swimming ? WaterCorridor{} : InspectWaterCorridor(start, target_pos);
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    if (Navigation::IsPositionDangerous(zones, target_pos)) {
        StopMovement();
        return;
    }
    bool started = false;
    if (swimming) {
        started = TrySwimMove(target_pos, true, false);
    } else {
        const float distance = start.DistanceTo2D(target_pos);
        if (water.entryNear) started = TryWaterEntryMove(target_pos, true);
        if (!started && !water.entryNear && distance <= 25.0f && !water.crosses &&
            HasDirectGroundRoute(start, target_pos, zones) &&
            IsPathSafeFromEnemies(start, target_pos))
            started = StartMovement(target_pos);
        if (!started && water.crosses && !water.entryNear)
            started = TryGroundMove(target_pos, true, false, true, water.groundHorizon);
        if (!started) started = TryServerWaypoint(target_pos, true);
    }
    CompleteMovement(started, moveType);
}

void Functions::MoveTo(Position target_pos, MoveTypes moveType, bool checkEnemyClose, bool targetSwim) {
    if (!localPlayer || (LateralRecovery.Active && moveType != MoveTypes::BossMechanic)) return;
    LateralRecovery.Active = false;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const bool meleeTarget = moveType == MoveTypes::GoToTarget && targetUnit &&
        targetUnit->attackable && !targetUnit->isdead &&
        target_pos.DistanceTo(targetUnit->position) < 2.0f &&
        !Functions::PlayerIsRanged();

    if (!ChooseMeleeApproach(target_pos, meleeTarget, checkEnemyClose, zones)) {
        StopMovement();
        return;
    }
    if (Navigation::IsPositionDangerous(zones, target_pos)) {
        StopMovement();
        return;
    }
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    const bool follow = moveType == MoveTypes::Follow;
    const unsigned long long ignoredGuid = moveType == MoveTypes::GoToTarget && targetUnit ?
        targetUnit->Guid : 0;
    const Position start = localPlayer->position;
    const float distanceToGoal = start.DistanceTo2D(target_pos);
    const auto water = swimming ? WaterCorridor{} : InspectWaterCorridor(start, target_pos);

    if (swimming) {
        CompleteMovement(TrySwimMove(target_pos, checkEnemyClose, false, ignoredGuid), moveType);
        return;
    }

    if (water.entryNear || (targetSwim && start.DistanceTo(target_pos) <= 5.0f)) {
        bool started = TryWaterEntryMove(target_pos, checkEnemyClose, ignoredGuid);
        if (!started && water.crosses)
            started = TryGroundMove(target_pos, checkEnemyClose, false, true,
                water.groundHorizon, ignoredGuid);
        if (!started)
            started = TryServerWaypoint(target_pos, checkEnemyClose, ignoredGuid);
        if (!started) started = TryLocalRecoveryStep(target_pos, checkEnemyClose, follow, ignoredGuid);
        CompleteMovement(started, moveType);
        return;
    }

    const bool steepHeightChange = !water.crosses &&
        std::abs(start.Z - target_pos.Z) >= 7.0f &&
        std::abs(start.Z - target_pos.Z) > distanceToGoal * 0.5f;
    const bool failedHeading = IsRecentlyFailedHeading(start, target_pos);
    const bool serverFirst = steepHeightChange || failedHeading ||
        (!follow && !water.crosses && distanceToGoal > 14.0f &&
         !Navigation::IsSegmentSafe(zones, start, target_pos));

    bool started = false;
    if (serverFirst) started = TryServerWaypoint(target_pos, checkEnemyClose, ignoredGuid);
    if (!started) started = TryGroundMove(target_pos, checkEnemyClose, false,
        (follow || water.crosses) && !steepHeightChange && !failedHeading,
        water.groundHorizon, ignoredGuid);
    if (!started && !serverFirst)
        started = TryServerWaypoint(target_pos, checkEnemyClose, ignoredGuid);

    if (!started && meleeTarget && !Navigation::IsSegmentSafe(zones, start, target_pos))
        started = TryCombatDetour(target_pos, checkEnemyClose, zones);

    if (!started) started = TryLocalRecoveryStep(target_pos, checkEnemyClose, follow, ignoredGuid);
    CompleteMovement(started, moveType);
}

void Functions::MoveToLoS(Position target_pos, MoveTypes moveType, bool targetSwim) {
    if (!localPlayer || (LateralRecovery.Active && moveType != MoveTypes::BossMechanic)) return;
    LateralRecovery.Active = false;
    const Position start = localPlayer->position;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    const auto water = swimming ? WaterCorridor{} : InspectWaterCorridor(start, target_pos);
    const unsigned long long ignoredGuid = targetUnit &&
        target_pos.DistanceTo(targetUnit->position) < 2.0f ? targetUnit->Guid : 0;

    bool started = false;
    if (swimming) {
        started = TrySwimMove(target_pos, true, true, ignoredGuid);
    } else {
        started = TryGroundMove(target_pos, true, true, false, 16.0f, ignoredGuid);
        if (!started && (water.entryNear ||
            (targetSwim && start.DistanceTo(target_pos) <= 5.0f)))
            started = TryWaterEntryMove(target_pos, true, ignoredGuid);
        if (!started && water.crosses && !water.entryNear)
            started = TryGroundMove(target_pos, true, false, true, water.groundHorizon, ignoredGuid);
        if (!started)
            started = TryServerWaypoint(target_pos, true, ignoredGuid);
    }
    if (!started && !swimming) started = TryLocalRecoveryStep(target_pos, true, false, ignoredGuid);
    CompleteMovement(started, moveType);
}
