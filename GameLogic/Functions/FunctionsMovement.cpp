#define NOMINMAX
#include "Functions.h"

#include "../MemoryManager.h"
#include "../Game.h"
#include "FunctionsLua.h"
#include "../rng.h"
#include "../Navigation.h"
#include "../FactionTemplate.h"

#include <iostream>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <tuple>
#include <vector>
#include <random>

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float ANGLE_STEP = PI / 8.0f;
    constexpr float MIN_MOVE_DISTANCE = 2.0f;
    constexpr float SWIM_STEP = 2.5f;
    constexpr float SWIM_VERTICAL_STEP = 1.0f;
    constexpr std::array<int, 13> MOVE_OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

    float Distance(const Position& a, const Position& b) {
        const float dx = a.X - b.X;
        const float dy = a.Y - b.Y;
        const float dz = a.Z - b.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float Distance2D(const Position& a, const Position& b) {
        const float dx = a.X - b.X;
        const float dy = a.Y - b.Y;
        return std::sqrt(dx * dx + dy * dy);
    }

    bool IsGroundSafe8Points(const Position& center, float radius) {
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

        for (int i = 0; i < POINT_COUNT; ++i) {
            Position probe(
                center.X + directions[i][0] * radius,
                center.Y + directions[i][1] * radius,
                center.Z
            );

            Position projected = Functions::ProjectPos(probe);
            if (Distance(projected, probe) >= 2.0f || Functions::Intersect(projected, center, 1.0f)) return false;
        }
        return true;
    }

    bool ProjectSafeGroundStep(const Position& from, const Position& wanted, Position& result,
        float groundCheckRadius, const Navigation::AvoidanceSnapshot& zones = {},
        bool checkDanger = true) {
        const auto currentZones = checkDanger ? (zones ? zones : Navigation::GetAvoidanceSnapshot(mapID)) : Navigation::AvoidanceSnapshot{};
        if (checkDanger && !Navigation::IsSegmentSafe(currentZones, from, wanted)) return false;
        result = Functions::ProjectPos(wanted);
        if (Distance(result, wanted) >= 2.0f) return false;
        if (Functions::Intersect(from, result, 1.0f)) return false;
        if (checkDanger && !Navigation::IsSegmentSafe(currentZones, from, result)) return false;
        return IsGroundSafe8Points(result, groundCheckRadius);
    }

    bool HasDirectGroundRoute(const Position& from, const Position& goal,
        const Navigation::AvoidanceSnapshot& zones,
        bool checkEnemyClose = false) {
        constexpr float STEP = 2.0f;
        constexpr int MAX_STEPS = 15;
        constexpr float MAX_GROUND_STEP = 1.0f;
        const float distance = Distance2D(from, goal);
        if (distance < 0.01f || distance > STEP * MAX_STEPS ||
            !Navigation::IsSegmentSafe(zones, from, goal) ||
            Functions::Intersect(from, goal, 1.0f)) return false;

        const int steps = static_cast<int>(std::ceil(distance / STEP));
        Position previous = from;
        for (int i = 1; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const Position wanted(from.X + (goal.X - from.X) * t,
                from.Y + (goal.Y - from.Y) * t,
                from.Z + (goal.Z - from.Z) * t);
            Position projected;
            if (!ProjectSafeGroundStep(previous, wanted, projected, 3.0f, zones) ||
                std::abs(projected.Z - previous.Z) > MAX_GROUND_STEP ||
                (checkEnemyClose && Functions::enemyClose(projected))) return false;
            previous = projected;
        }
        return true;
    }

    bool HasValidatedPivot(const Position& start, const Position& pivot, const Position& goal,
        const Navigation::AvoidanceSnapshot& zones) {
        if (!HasDirectGroundRoute(start, pivot, zones)) return false;
        const float distance = Distance2D(start, pivot);
        if (distance < MIN_MOVE_DISTANCE) return false;
        const float t = (std::max)(0.0f, (distance - 1.5f) / distance);
        const Position approach(start.X + (pivot.X - start.X) * t,
            start.Y + (pivot.Y - start.Y) * t,
            start.Z + (pivot.Z - start.Z) * t);
        const Position groundApproach = Functions::ProjectPos(approach);
        if (Distance(groundApproach, approach) >= 1.0f) return false;
        return HasDirectGroundRoute(groundApproach, goal, zones);
    }

    struct FailedHeading {
        Position Origin;
        Position Destination;
        int MapId;
        std::chrono::steady_clock::time_point Since;
    };
    std::vector<FailedHeading> FailedHeadings;

    struct ActiveGoal {
        Position Goal{};
        int Map = -1;
        MoveTypes Type = MoveTypes::NotMoving;
        bool Valid = false;
    };
    ActiveGoal CurrentGoal;

    bool ReuseActiveWaypoint(const Position& goal, MoveTypes type) {
        if (!CurrentGoal.Valid || CurrentGoal.Map != mapID || CurrentGoal.Type != type ||
            Moving != type || Distance2D(CurrentGoal.Goal, goal) > 2.5f) return false;
        if (!(localPlayer->movement_flags & MOVEFLAG_FORWARD) && !localPlayer->isMoving) return false;
        Position waypoint;
        if (!Navigation::GetCurrentMovementDestination(waypoint) ||
            Distance2D(localPlayer->position, waypoint) <= 1.5f ||
            Navigation::IsCurrentMovementStalled(localPlayer->position) ||
            Navigation::IsCurrentMovementUnsafe(mapID, localPlayer->position)) return false;
        return true;
    }

    void RememberGoal(const Position& goal, MoveTypes type) {
        CurrentGoal = { goal, mapID, type, true };
    }

    bool IsRecentlyFailedHeading(const Position& start, const Position& candidate) {
        const auto now = std::chrono::steady_clock::now();
        FailedHeadings.erase(std::remove_if(FailedHeadings.begin(), FailedHeadings.end(),
            [now](const FailedHeading& failure) {
                return now - failure.Since > std::chrono::seconds(8);
            }), FailedHeadings.end());

        const float dx = candidate.X - start.X, dy = candidate.Y - start.Y;
        const float length = std::hypot(dx, dy);
        if (length < MIN_MOVE_DISTANCE) return false;
        for (const auto& failure : FailedHeadings) {
            if (failure.MapId != mapID || Distance2D(start, failure.Origin) > 5.0f) continue;
            const float fx = failure.Destination.X - failure.Origin.X;
            const float fy = failure.Destination.Y - failure.Origin.Y;
            const float flength = std::hypot(fx, fy);
            if (flength > MIN_MOVE_DISTANCE && (dx * fx + dy * fy) / (length * flength) > 0.85f)
                return true;
        }
        return false;
    }

    bool SafeClickToMove(const Position& destination, unsigned long long guid, bool emergency = false) {
        if (!emergency && IsRecentlyFailedHeading(localPlayer->position, destination)) return false;
        if (!Navigation::IsSegmentSafe(mapID, localPlayer->position, destination)) return false;
        localPlayer->ClickToMove(Move, guid, destination);
        Navigation::RecordMovement(localPlayer->position, destination);
        return true;
    }

    void resetMovement() {
        CurrentGoal.Valid = false;
        Navigation::ClearMovement();
        if (localPlayer->isMoving || (localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
            ThreadSynchronizer::pressKey(0x28);
            ThreadSynchronizer::releaseKey(0x28);
        }
        else if (localPlayer->movement_flags & MOVEFLAG_BACKWARD) {
            ThreadSynchronizer::releaseKey(0x28);
        }
        Moving = MoveTypes::NotMoving;
    }

    struct LateralRecoveryState {
        bool Active = false;
        int MapId = -1;
        MoveTypes Type = MoveTypes::NotMoving;
        int LastSide = 0;
        Position Destination{};
        std::chrono::steady_clock::time_point Expires{};
    };
    LateralRecoveryState LateralRecovery;

    Position BuildSwimStep(const Position& from, const Position& target, float cosDir, float sinDir, float step) {
        const float dz = std::clamp(target.Z - from.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
        return Position(from.X + cosDir * step, from.Y + sinDir * step, from.Z + dz);
    }

    bool TryLocalRecoveryStep(const Position& destination, bool checkEnemyClose) {
        const Position start = localPlayer->position;
        const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
        if (Navigation::IsPositionDangerous(zones, destination) ||
            Distance2D(start, destination) < MIN_MOVE_DISTANCE) return false;

        const float base = std::atan2(destination.Y - start.Y, destination.X - start.X);
        const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
        for (const float stride : {3.0f, 5.0f, 8.0f}) {
            bool found = false;
            float bestScore = 1.0e9f;
            Position best;
            for (int off : MOVE_OFFSETS) {
                const float dir = base + off * ANGLE_STEP;
                const float cosDir = std::cos(dir), sinDir = std::sin(dir);
                Position wanted(start.X + cosDir * stride,
                    start.Y + sinDir * stride, start.Z);
                if (IsRecentlyFailedHeading(start, wanted) ||
                    !Navigation::IsSegmentSafe(zones, start, wanted)) continue;
                Position next;
                if (swimming) {
                    next = BuildSwimStep(start, destination, cosDir, sinDir, stride);
                    if (Functions::Intersect(start, next, 1.25f)) continue;
                }
                else if (!ProjectSafeGroundStep(start, wanted, next, 3.0f, zones)) continue;

                if (!Navigation::IsSegmentSafe(zones, start, next) ||
                    IsRecentlyFailedHeading(start, next) ||
                    Distance(start, next) < MIN_MOVE_DISTANCE ||
                    (checkEnemyClose && Functions::enemyClose(next)) ||
                    (!swimming && !HasValidatedPivot(start, next, destination, zones))) continue;
                const float score = Distance2D(next, destination) +
                    std::abs(off) * 0.08f + stride * 0.08f;
                if (score < bestScore) {
                    best = next;
                    bestScore = score;
                    found = true;
                }
            }
            if (found && SafeClickToMove(best, localPlayer->Guid)) return true;
        }
        return false;
    }
}

bool Functions::EscapeDangerZone() {
    const Position start = localPlayer->position;
    if (!Navigation::IsPositionDangerous(mapID, start)) return false;

    if (Moving == MoveTypes::BossMechanic && localPlayer->isMoving &&
        !Navigation::IsCurrentMovementUnsafe(mapID, start)) return true;

    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const DangerZone* nearest = nullptr;
    float greatestPenetration = -1.0e9f;
    for (const auto& zone : *zones) {
        const float penetration = zone.Radius + 1.5f - Distance2D(start, zone.Center);
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
            if (Distance(start, candidate) < MIN_MOVE_DISTANCE) continue;
            float clearance = 1.0e9f;
            for (const auto& zone : *zones) {
                if (std::abs(candidate.Z - zone.Center.Z) <= 6.0f) {
                    clearance = (std::min)(clearance,
                        Distance2D(candidate, zone.Center) - zone.Radius - 1.5f);
                }
            }
            const float score = clearance - stride * 0.04f - std::abs(off) * 0.05f -
                (Functions::enemyClose(candidate) ? 5.0f : 0.0f);
            if (score > bestScore) {
                best = candidate;
                bestScore = score;
                found = true;
            }
        }
    }
    if (found && SafeClickToMove(best, localPlayer->Guid, true)) Moving = MoveTypes::BossMechanic;
    else resetMovement();
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
        resetMovement();
        return false;
    }
    const bool arrived = Distance2D(localPlayer->position, LateralRecovery.Destination) <= 0.85f;
    const bool expired = std::chrono::steady_clock::now() >= LateralRecovery.Expires;
    if (arrived || expired) {
        LateralRecovery.Active = false;
        resetMovement();
        return false;
    }
    return true;
}

void Functions::HandleMovementStall() {
    if (LateralRecovery.Active || localPlayer == nullptr) return;
    Position failedDestination;
    const MoveTypes previousType = Moving;
    const bool hasFailedWaypoint = Navigation::GetCurrentMovementDestination(failedDestination) &&
        Distance2D(localPlayer->position, failedDestination) >= MIN_MOVE_DISTANCE;
    if (hasFailedWaypoint) {
        FailedHeadings.push_back({ localPlayer->position, failedDestination, mapID,
                                  std::chrono::steady_clock::now() });
        if (FailedHeadings.size() > 8) FailedHeadings.erase(FailedHeadings.begin());
    }
    resetMovement();
    if (!hasFailedWaypoint || previousType == MoveTypes::Follow || previousType == MoveTypes::Journey ||
        previousType == MoveTypes::NotMoving || previousType == MoveTypes::BossMechanic ||
        (localPlayer->movement_flags & (MOVEFLAG_ROOT | MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR |
            MOVEFLAG_SWIMMING | MOVEFLAG_FLYING | MOVEFLAG_FLYING2)) ||
        !std::isfinite(localPlayer->facing)) return;

    constexpr float SIDE_DISTANCE = 2.5f;
    constexpr float SAMPLE_STEP = 0.5f;
    constexpr float BODY_MARGIN = 0.5f;
    constexpr float MAX_HEIGHT_FROM_START = 0.8f;
    constexpr float MAX_STEP_HEIGHT = 0.45f;

    const Position start = localPlayer->position;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const float leftX = -std::sin(localPlayer->facing);
    const float leftY = std::cos(localPlayer->facing);
    const int preferred = LateralRecovery.LastSide == 1 ? -1 : 1;
    for (int side : {preferred, -preferred}) {
        const float dirX = leftX * side, dirY = leftY * side;
        const float forwardX = -dirY, forwardY = dirX;
        Position previous = start;
        Position candidate;
        bool safe = true;
        for (float distance = SAMPLE_STEP; distance <= SIDE_DISTANCE + 0.75f; distance += SAMPLE_STEP) {
            const Position point(start.X + dirX * distance, start.Y + dirY * distance, start.Z);
            const Position ground = Functions::ProjectPos(point);
            if (!std::isfinite(ground.Z) ||
                std::abs(ground.Z - start.Z) > MAX_HEIGHT_FROM_START ||
                std::abs(ground.Z - previous.Z) > MAX_STEP_HEIGHT ||
                !Navigation::IsSegmentSafe(zones, previous, ground) ||
                Functions::Intersect(previous, ground, 1.0f)) {
                safe = false;
                break;
            }
            for (float offset : {-BODY_MARGIN, BODY_MARGIN}) {
                const Position edge(point.X + forwardX * offset,
                    point.Y + forwardY * offset, start.Z);
                const Position edgeGround = Functions::ProjectPos(edge);
                if (!std::isfinite(edgeGround.Z) ||
                    std::abs(edgeGround.Z - ground.Z) > MAX_STEP_HEIGHT ||
                    Navigation::IsPositionDangerous(zones, edgeGround)) {
                    safe = false;
                    break;
                }
            }
            if (!safe) break;
            if (distance <= SIDE_DISTANCE) candidate = ground;
            previous = ground;
        }
        if (!safe || IsRecentlyFailedHeading(start, candidate) ||
            !SafeClickToMove(candidate, localPlayer->Guid)) continue;
        LateralRecovery.Active = true;
        LateralRecovery.MapId = mapID;
        LateralRecovery.Type = previousType;
        LateralRecovery.LastSide = side;
        LateralRecovery.Destination = candidate;
        LateralRecovery.Expires = std::chrono::steady_clock::now() + std::chrono::milliseconds(1100);
        Moving = previousType;
        return;
    }
}

bool Functions::StepBack(WoWUnit* target, MoveTypes MoveType, float dist_away) {
    if (!target || !localPlayer) return false;

    const Position start = localPlayer->position;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const auto avoidsUnengagedEnemies = [&](const Position& destination) {
        const float dx = destination.X - start.X;
        const float dy = destination.Y - start.Y;
        const float dz = destination.Z - start.Z;
        const float segmentLengthSq = dx * dx + dy * dy + dz * dz;
        const float segmentLengthXYSq = dx * dx + dy * dy;
        const float bx = target->position.X - start.X;
        const float by = target->position.Y - start.Y;
        const float targetT = segmentLengthXYSq > 0.0f ?
            std::clamp((bx * dx + by * dy) / segmentLengthXYSq, 0.0f, 1.0f) : 0.0f;
        const float nearestTargetX = bx - dx * targetT;
        const float nearestTargetY = by - dy * targetT;
        if (nearestTargetX * nearestTargetX + nearestTargetY * nearestTargetY + 0.01f < bx * bx + by * by)
            return false;

        for (auto& unit : ListUnits) {
            if (unit.Guid == target->Guid || !unit.attackable || unit.isdead ||
                unit.isInCombatOrEncounter() || unit.unitReaction >= Neutral ||
                FactionTemplate.isNeutral(unit.factionTemplateID) ||
                (unit.flags & UNIT_FLAG_PLAYER_CONTROLLED) || unit.creatureType == Totem ||
                ((unit.movement_flags & MOVEFLAG_SWIMMING) && !(localPlayer->movement_flags & MOVEFLAG_SWIMMING))) continue;

            const float radius = 21.0f + static_cast<float>(unit.level - localPlayer->level);
            if (radius <= 0.0f) continue;

            const float ex = unit.position.X - start.X;
            const float ey = unit.position.Y - start.Y;
            const float ez = unit.position.Z - start.Z;
            const float t = segmentLengthSq > 0.0f ?
                std::clamp((ex * dx + ey * dy + ez * dz) / segmentLengthSq, 0.0f, 1.0f) : 0.0f;
            const float px = ex - dx * t;
            const float py = ey - dy * t;
            const float pz = ez - dz * t;
            const float nearestDistanceSq = px * px + py * py + pz * pz;
            const float startDistanceSq = ex * ex + ey * ey + ez * ez;

            if (std::abs(pz) < 20.0f && nearestDistanceSq < radius * radius &&
                (startDistanceSq >= radius * radius || nearestDistanceSq + 0.01f < startDistanceSq))
                return false;
        }
        return true;
        };

    Position currentDestination;
    if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) && Moving == MoveType &&
        Navigation::HasCurrentMovement() &&
        !Navigation::IsCurrentMovementStalled(start) &&
        !Navigation::IsCurrentMovementUnsafe(mapID, start) &&
        Navigation::GetCurrentMovementDestination(currentDestination) &&
        target->position.DistanceTo(currentDestination) - localPlayer->combatReach - target->combatReach >= dist_away &&
        avoidsUnengagedEnemies(currentDestination)) {
        return true;
    }

    std::vector<Position> list_pos;
    list_pos.reserve(32);
    static const std::array<std::array<float, 2>, 32> directions = [] {
        std::array<std::array<float, 2>, 32> values{};
        const float halfPI = acosf(0);
        for (int i = 0; i < 32; ++i) {
            const float dir = i * halfPI / 8;
            values[i] = { std::cos(dir) * 2.0f, std::sin(dir) * 2.0f };
        }
        return values;
        }();

    for (int i = 0; i < 32; i++) {
        const float stepX = directions[i][0];
        const float stepY = directions[i][1];
        Position last_pos = target->position;

        for (int w = 0; w < 10; w++) {
            Position tmp_pos(last_pos.X + stepX, last_pos.Y + stepY, last_pos.Z);
            Position next_pos = tmp_pos;

            if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) {
                next_pos = Functions::ProjectPos(tmp_pos);
                if (Distance(next_pos, tmp_pos) >= 2.0f || !IsGroundSafe8Points(next_pos, 3.0f)) break;
            }

            if (!Functions::Intersect(last_pos, next_pos) && Navigation::IsSegmentSafe(zones, last_pos, next_pos)) {
                if ((target->position.DistanceTo(next_pos) - localPlayer->combatReach - target->combatReach) >= dist_away &&
                    !Functions::Intersect(next_pos, target->position)) {
                    list_pos.push_back(next_pos);
                    break;
                }
                last_pos = next_pos;
                continue;
            }
            break;
        }
    }

    std::sort(list_pos.begin(), list_pos.end(), [&](const Position& a, const Position& b) {
        return Distance(a, start) < Distance(b, start);
        });

    for (const Position& position : list_pos) {
        const Position randomized = Functions::RandomisePos(position, 2.0f, target->position,
            dist_away + localPlayer->combatReach + target->combatReach);

        for (const Position& candidate : { randomized, position }) {
            if (Distance(candidate, start) < MIN_MOVE_DISTANCE ||
                !avoidsUnengagedEnemies(candidate) ||
                ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) ?
                    (Functions::Intersect(start, candidate) || !Navigation::IsSegmentSafe(zones, start, candidate)) :
                    !HasDirectGroundRoute(start, candidate, zones)) ||
                !SafeClickToMove(candidate, target->Guid)) continue;
            Moving = MoveType;
            return true;
        }
    }

    resetMovement();
    return false;
}

void Functions::FollowMultibox(int placement) {
    if (Leader == NULL) return;

    float range = 2.0f;
    float cst = 0.30f;

    if (placement == 1) cst = 0.30f;
    else if (placement == 2) cst = -0.30f;
    else if (placement == 3) {
        range = 4.0f;
        cst = localPlayer->isMounted ? 0.45f : 0.15f;
    }
    else if (placement == 4) {
        range = 4.0f;
        cst = localPlayer->isMounted ? -0.45f : -0.15f;
    }

    const float halfPI = acosf(0);
    Position target_pos((cos(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.X, (sin(Leader->facing + (halfPI * 2) + cst) * range) + Leader->position.Y, Leader->position.Z);
    const bool targetSwim = Leader->movement_flags & MOVEFLAG_SWIMMING;

    ThreadSynchronizer::RunOnMainThread([=]() {
        Position projected_pos;
        ProjectSafeGroundStep(Leader->position, target_pos, projected_pos, 3.0f, {}, false);
        if (!targetSwim && projected_pos.DistanceTo(target_pos) > (2.0f + range)) {
            resetMovement();
            return;
        }

        Functions::MoveToFast(target_pos, MoveTypes::Follow);
        });
}

void Functions::MoveToFast(Position target_pos, MoveTypes moveType) {
    if (!Functions::Intersect(localPlayer->position, target_pos)) {
        localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
        Moving = moveType;
    }
    else {
        Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos);

        if (nextpos.DistanceTo(localPlayer->position) >= MIN_MOVE_DISTANCE && !Functions::enemyClose(nextpos)) {
            localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
            Moving = moveType;
        }
        else resetMovement();
    }
}

void Functions::MoveTo(Position target_pos, MoveTypes moveType, bool checkEnemyClose, bool targetSwim) {
    if (LateralRecovery.Active) {
        if (moveType != MoveTypes::BossMechanic) return;
        LateralRecovery.Active = false;
    }
    if (Moving == MoveTypes::Follow || Moving == MoveTypes::Journey) Navigation::ClearMovement();
    if (Navigation::IsPositionDangerous(mapID, target_pos)) {
        resetMovement();
        return;
    }
    if (ReuseActiveWaypoint(target_pos, moveType)) return;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;

    if (swimming) {
        if (Functions::MoveObstacleSwim(target_pos, checkEnemyClose) ||
            TryLocalRecoveryStep(target_pos, checkEnemyClose)) {
            Moving = moveType;
            RememberGoal(target_pos, moveType);
        }
        else resetMovement();
        return;
    }

    if (targetSwim) {
        const Position start = localPlayer->position;
        const float targetDist = Distance2D(start, target_pos);
        bool started = false;
        if (targetDist >= MIN_MOVE_DISTANCE) {
            const float step = targetDist <= SWIM_STEP ? targetDist : SWIM_STEP;
            const float base = std::atan2(target_pos.Y - start.Y, target_pos.X - start.X);

            for (int off : MOVE_OFFSETS) {
                const float dir = base + off * ANGLE_STEP;
                const float cosDir = std::cos(dir), sinDir = std::sin(dir);
                const Position wanted(start.X + cosDir * step, start.Y + sinDir * step, start.Z);
                const Position projected = Functions::ProjectPos(wanted);
                Position next = wanted;

                if (Distance(projected, wanted) < 2.0f) next = projected;
                else {
                    const float dz = std::clamp(target_pos.Z - start.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
                    next.Z = start.Z + dz;
                }

                if (Distance(start, next) < MIN_MOVE_DISTANCE ||
                    Functions::Intersect(start, next, 1.25f) ||
                    (checkEnemyClose && Functions::enemyClose(next))) continue;
                if (SafeClickToMove(next, localPlayer->Guid)) {
                    started = true;
                    break;
                }
            }
        }
        if (started || TryLocalRecoveryStep(target_pos, checkEnemyClose)) {
            Moving = moveType;
            RememberGoal(target_pos, moveType);
        }
        else resetMovement();
        return;
    }

    const auto start = localPlayer->position;
    const float distanceToGoal = Distance2D(start, target_pos);
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const bool serverFirst = distanceToGoal > 14.0f &&
        !Navigation::IsSegmentSafe(zones, start, target_pos);
    auto tryServer = [&]() -> bool {
        const Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos, true);
        if (Distance(nextpos, localPlayer->position) < MIN_MOVE_DISTANCE ||
            (checkEnemyClose && Functions::enemyClose(nextpos)) ||
            !Navigation::IsSegmentSafe(zones, localPlayer->position, nextpos)) return false;
        if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) &&
            Navigation::IsMovingTowards(nextpos) &&
            !Navigation::IsCurrentMovementStalled(localPlayer->position)) return true;
        return SafeClickToMove(nextpos, localPlayer->Guid);
        };

    bool started = false;
    if (serverFirst) started = tryServer();
    if (!started) started = Functions::MoveObstacle(target_pos, checkEnemyClose);
    if (!started && !serverFirst) started = tryServer();
    if (!started) started = TryLocalRecoveryStep(target_pos, checkEnemyClose);
    if (started) {
        Moving = moveType;
        RememberGoal(target_pos, moveType);
    }
    else resetMovement();
}

void Functions::MoveToLoS(Position target_pos, MoveTypes moveType) {
    if (LateralRecovery.Active) {
        if (moveType != MoveTypes::BossMechanic) return;
        LateralRecovery.Active = false;
    }
    if (Moving == MoveTypes::Follow || Moving == MoveTypes::Journey) Navigation::ClearMovement();
    if (Navigation::IsPositionDangerous(mapID, target_pos)) { resetMovement(); return; }
    if (ReuseActiveWaypoint(target_pos, moveType)) return;
    if (localPlayer->movement_flags & MOVEFLAG_SWIMMING) {
        if (Functions::MoveLoSSwim(target_pos) || TryLocalRecoveryStep(target_pos, true)) {
            Moving = moveType;
            RememberGoal(target_pos, moveType);
        }
        else resetMovement();
        return;
    }

    if (!Functions::MoveLoS(target_pos)) {
        Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos, true);
        if (nextpos.DistanceTo(localPlayer->position) >= MIN_MOVE_DISTANCE &&
            !Functions::enemyClose(nextpos) &&
            Navigation::IsSegmentSafe(mapID, localPlayer->position, nextpos)) {
            if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) &&
                Navigation::IsMovingTowards(nextpos) &&
                !Navigation::IsCurrentMovementStalled(localPlayer->position)) {
                Moving = moveType;
                RememberGoal(target_pos, moveType);
                return;
            }
            if (SafeClickToMove(nextpos, localPlayer->Guid)) {
                Moving = moveType;
                RememberGoal(target_pos, moveType);
                return;
            }
        }
        if (TryLocalRecoveryStep(target_pos, true)) {
            Moving = moveType;
            RememberGoal(target_pos, moveType);
        }
        else resetMovement();
    }
    else {
        Moving = moveType;
        RememberGoal(target_pos, moveType);
    }
}

bool Functions::MoveLoS(Position target_pos) {
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    constexpr float STEP = 2.5f;
    constexpr int MAX_STEPS = 12;

    const float dx = target_pos.X - localPlayer->position.X;
    const float dy = target_pos.Y - localPlayer->position.Y;
    const float base = std::atan2(dy, dx);

    for (int off : MOVE_OFFSETS) {
        const float dir = base + off * ANGLE_STEP;
        const float stepX = std::cos(dir) * STEP;
        const float stepY = std::sin(dir) * STEP;
        Position last = localPlayer->position;

        for (int s = 0; s < MAX_STEPS; s++) {
            Position stepPos(last.X + stepX, last.Y + stepY, last.Z);
            Position next;

            if (!ProjectSafeGroundStep(last, stepPos, next, 3.0f, zones)) break;
            if (Functions::enemyClose(next)) break;

            if (!Functions::Intersect(next, target_pos)) {
                if (Distance(localPlayer->position, next) < MIN_MOVE_DISTANCE) break;
                if (SafeClickToMove(next, localPlayer->Guid)) return true;
                break;
            }

            last = next;
        }
    }

    return false;
}

bool Functions::MoveLoSSwim(Position target_pos) {
    if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) return false;

    const Position start = localPlayer->position;
    const float targetDist = Distance2D(start, target_pos);
    if (targetDist < MIN_MOVE_DISTANCE) return !Functions::Intersect(start, target_pos);

    const float step = targetDist <= SWIM_STEP ? targetDist : SWIM_STEP;
    const float base = std::atan2(target_pos.Y - start.Y, target_pos.X - start.X);
    Position fallback = start;
    bool hasFallback = false;

    for (int off : MOVE_OFFSETS) {
        const float dir = base + off * ANGLE_STEP;
        Position next = BuildSwimStep(start, target_pos, std::cos(dir), std::sin(dir), step);

        if (Distance(start, next) < MIN_MOVE_DISTANCE) continue;
        if (Functions::Intersect(start, next, 1.25f) || Functions::enemyClose(next)) continue;

        if (!Functions::Intersect(next, target_pos)) {
            if (!SafeClickToMove(next, localPlayer->Guid)) continue;
            return true;
        }

        if (!hasFallback) {
            fallback = next;
            hasFallback = true;
        }
    }

    if (!hasFallback) return false;

    return SafeClickToMove(fallback, localPlayer->Guid);
}

bool Functions::MoveObstacle(Position target_pos, bool checkEnemyClose) {
    Position start = localPlayer->position;
    if (start.DistanceTo(target_pos) > 40.0f) return false;
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    if (Navigation::IsPositionDangerous(zones, target_pos)) return false;

    if (HasDirectGroundRoute(start, target_pos, zones, checkEnemyClose))
        return Distance(start, target_pos) >= MIN_MOVE_DISTANCE &&
        SafeClickToMove(target_pos, localPlayer->Guid);

    constexpr float STEP = 2.0f;
    constexpr int MAX_STEPS = 15;
    const float dx = target_pos.X - start.X;
    const float dy = target_pos.Y - start.Y;
    const float base = std::atan2(dy, dx);

    for (int off : MOVE_OFFSETS) {
        const float dir = base + off * ANGLE_STEP;
        const float stepX = std::cos(dir) * STEP;
        const float stepY = std::sin(dir) * STEP;
        Position last = start;

        for (int s = 0; s < MAX_STEPS; s++) {
            const Position stepPos(last.X + stepX, last.Y + stepY, last.Z);
            Position next;
            if (!ProjectSafeGroundStep(last, stepPos, next, 3.0f, zones)) break;
            if (checkEnemyClose && Functions::enemyClose(next)) break;

            if (Distance2D(start, next) >= MIN_MOVE_DISTANCE &&
                !IsRecentlyFailedHeading(start, next) &&
                HasDirectGroundRoute(next, target_pos, zones) &&
                HasValidatedPivot(start, next, target_pos, zones) &&
                SafeClickToMove(next, localPlayer->Guid)) return true;

            last = next;
        }
    }
    return false;
}

bool Functions::MoveObstacleSwim(Position target_pos, bool checkEnemyClose) {
    if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) return false;

    const Position start = localPlayer->position;
    const float targetDist = Distance2D(start, target_pos);
    if (targetDist < MIN_MOVE_DISTANCE) return false;

    const float step = targetDist <= SWIM_STEP ? targetDist : SWIM_STEP;
    const float base = std::atan2(target_pos.Y - start.Y, target_pos.X - start.X);

    for (int off : MOVE_OFFSETS) {
        const float dir = base + off * ANGLE_STEP;
        Position next = BuildSwimStep(start, target_pos, std::cos(dir), std::sin(dir), step);

        if (Distance(start, next) < MIN_MOVE_DISTANCE) continue;
        if (Functions::Intersect(start, next, 1.25f)) continue;
        if (checkEnemyClose && Functions::enemyClose(next)) continue;

        if (!SafeClickToMove(next, localPlayer->Guid)) continue;
        return true;
    }

    return false;
}
