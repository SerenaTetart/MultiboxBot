#define NOMINMAX
#include "Functions.h"

#include "../MemoryManager.h"
#include "../Game.h"
#include "FunctionsLua.h"
#include "../rng.h"
#include "../Navigation.h"
#include "../FactionTemplate.h"
#include "../DangerZoneGeometry.h"
#include "../WaterMap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <vector>

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float ANGLE_STEP = PI / 8.0f;
    constexpr float MIN_MOVE_DISTANCE = 2.0f;
    constexpr float SWIM_STEP = 2.5f;
    constexpr float SWIM_VERTICAL_STEP = 1.0f;
    constexpr std::array<int, 13> MOVE_OFFSETS = { 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6 };

    Position ClosestPointOnSegment(const Position& point, const Position& start, const Position& end) {
        const float dx = end.X - start.X, dy = end.Y - start.Y, dz = end.Z - start.Z;
        const float lengthSq = dx * dx + dy * dy + dz * dz;
        if (lengthSq < 0.000001f) return start;
        const float t = std::clamp(((point.X - start.X) * dx +
            (point.Y - start.Y) * dy + (point.Z - start.Z) * dz) / lengthSq, 0.0f, 1.0f);
        return Position(start.X + t * dx, start.Y + t * dy, start.Z + t * dz);
    }

    bool IsPathSafeFromEnemies(const Position& start, const Position& end,
        unsigned long long ignoredGuid = 0) {
        for (auto& enemy : ListUnits) {
            if (enemy.Guid == ignoredGuid || !enemy.attackable || enemy.isdead ||
                enemy.unitReaction >= Neutral ||
                (enemy.flags & UNIT_FLAG_PLAYER_CONTROLLED) || enemy.creatureType == Totem || enemy.createdBy != 0 ||
                ((enemy.movement_flags & MOVEFLAG_SWIMMING) &&
                    !(localPlayer->movement_flags & MOVEFLAG_SWIMMING))) continue;
            const float radius = 21.0f + static_cast<float>(enemy.level - localPlayer->level);
            if (radius <= 0.0f) continue;
            const Position nearest = ClosestPointOnSegment(enemy.position, start, end);
            const float current = enemy.position.DistanceTo(start);
            const float closest = enemy.position.DistanceTo(nearest);
            if (std::abs(enemy.position.Z - nearest.Z) < 20.0f && closest < radius &&
                (current >= radius || closest + 0.001f < current) &&
                !enemy.isInCombatOrEncounter() &&
                !FactionTemplate.isNeutral(enemy.factionTemplateID) &&
                !Functions::Intersect(enemy.position, nearest)) return false;
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

    struct ActiveGoal {
        Position Goal{};
        int Map = -1;
        MoveTypes Type = MoveTypes::NotMoving;
        bool Swimming = false;
        bool TargetSwimming = false;
        bool WaterOnRoute = false;
        bool WaterEntry = false;
        bool NavigationWaypoint = false;
        bool Valid = false;
    };
    ActiveGoal CurrentGoal;

    struct MeleeApproach {
        unsigned long long Guid = 0;
        int Map = -1;
        Position Boss{};
        Position Goal{};
        bool Swimming = false;
        bool Valid = false;
    };
    MeleeApproach LastMeleeApproach;

    struct CombatDetour {
        unsigned long long Guid = 0;
        int Map = -1;
        int Side = 0;
        Position Center{};
        Position Goal{};
    };
    CombatDetour LastCombatDetour;

    bool ReuseActiveWaypoint(const Position& goal, MoveTypes type, bool targetSwimming = false,
        bool waterOnRoute = false, bool waterEntry = false) {
        const bool combatReposition = type == MoveTypes::GoToTarget ||
            type == MoveTypes::MoveLoS || type == MoveTypes::MoveLoSAoE || type == MoveTypes::MoveLoSAlly;
        const float goalTolerance = type == MoveTypes::Follow ?
            (CurrentGoal.NavigationWaypoint ? 24.0f : 6.0f) :
            (combatReposition && !CurrentGoal.NavigationWaypoint ? 1.0f : 2.5f);
        if (!CurrentGoal.Valid || CurrentGoal.Map != mapID || CurrentGoal.Type != type ||
            Moving != type || CurrentGoal.Goal.DistanceTo2D(goal) > goalTolerance ||
            CurrentGoal.Swimming != bool(localPlayer->movement_flags & MOVEFLAG_SWIMMING) ||
            CurrentGoal.TargetSwimming != targetSwimming) return false;
        if (!CurrentGoal.NavigationWaypoint &&
            (CurrentGoal.WaterOnRoute != waterOnRoute || CurrentGoal.WaterEntry != waterEntry))
            return false;
        if (!CurrentGoal.NavigationWaypoint &&
            !(localPlayer->movement_flags & MOVEFLAG_FORWARD) && !localPlayer->isMoving)
            return false;
        Position waypoint;
        if (!Navigation::GetCurrentMovementDestination(waypoint) ||
            localPlayer->position.DistanceTo2D(waypoint) <= 1.5f ||
            Navigation::IsCurrentMovementStalled(localPlayer->position) ||
            Navigation::IsCurrentMovementUnsafe(mapID, localPlayer->position)) return false;
        if (type == MoveTypes::Follow && !CurrentGoal.NavigationWaypoint) {
            const Position& position = localPlayer->position;
            const float goalX = goal.X - position.X, goalY = goal.Y - position.Y;
            const float previousX = CurrentGoal.Goal.X - position.X;
            const float previousY = CurrentGoal.Goal.Y - position.Y;
            const float goalDistance = std::hypot(goalX, goalY);
            const float previousDistance = std::hypot(previousX, previousY);
            if (goalDistance < 2.5f || previousDistance < 2.5f ||
                (goalX * previousX + goalY * previousY) < 0.75f * goalDistance * previousDistance ||
                std::abs(previousX * goalY - previousY * goalX) > 3.5f * previousDistance)
                return false;
        }
        return true;
    }

    void RememberGoal(const Position& goal, MoveTypes type, bool targetSwimming = false,
        bool waterOnRoute = false, bool waterEntry = false) {
        CurrentGoal = { goal, mapID, type,
            bool(localPlayer->movement_flags & MOVEFLAG_SWIMMING), targetSwimming,
            waterOnRoute, waterEntry, CurrentGoal.NavigationWaypoint, true };
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
            if (flength > MIN_MOVE_DISTANCE && (dx * fx + dy * fy) / (length * flength) > 0.85f)
                return true;
        }
        return false;
    }

    bool SafeClickToMove(const Position& destination, unsigned long long guid,
        bool emergency = false, bool navigationWaypoint = false) {
        return Functions::StartMovement(destination, emergency, navigationWaypoint, guid);
    }

    bool TryServerWaypoint(const Position& goal, bool avoidEnemies,
        bool followFallback = false, unsigned long long ignoredGuid = 0,
        bool combatReplan = false) {
        const Position start = localPlayer->position;
        static std::chrono::steady_clock::time_point nextFailedFollowRetry{};
        static Position lastFailedFollowStart{};
        static Position lastFailedFollowGoal{};
        static int lastFailedFollowMap = -1;
        const auto now = std::chrono::steady_clock::now();
        if (followFallback && lastFailedFollowMap == mapID &&
            now < nextFailedFollowRetry &&
            start.DistanceTo2D(lastFailedFollowStart) < 3.0f &&
            goal.DistanceTo2D(lastFailedFollowGoal) < 8.0f) return false;

        const Position waypoint = Navigation::CalculatePath(mapID, start, goal, !combatReplan);
        bool accepted = start.DistanceTo(waypoint) >= MIN_MOVE_DISTANCE &&
            (!avoidEnemies || IsPathSafeFromEnemies(start, waypoint, ignoredGuid)) &&
            Navigation::IsSegmentSafe(mapID, start, waypoint);
        if (accepted && (localPlayer->movement_flags & MOVEFLAG_FORWARD) &&
            Navigation::IsMovingTowards(waypoint) &&
            !Navigation::IsCurrentMovementStalled(start)) return true;
        if (accepted)
            accepted = SafeClickToMove(waypoint, localPlayer->Guid, false, true);
        if (followFallback) {
            if (accepted) {
                nextFailedFollowRetry = {};
            } else {
                nextFailedFollowRetry = now + std::chrono::milliseconds(350);
                lastFailedFollowStart = start;
                lastFailedFollowGoal = goal;
                lastFailedFollowMap = mapID;
            }
        }
        return accepted;
    }

    bool ContinueDetour(const Position& goal, MoveTypes type, bool avoidEnemies,
        bool targetSwimming, bool waterOnRoute, bool waterEntry,
        unsigned long long ignoredGuid = 0) {
        if (!CurrentGoal.Valid || !CurrentGoal.NavigationWaypoint ||
            CurrentGoal.Map != mapID || CurrentGoal.Type != type ||
            (Moving != type && Moving != MoveTypes::NotMoving) ||
            CurrentGoal.Swimming != bool(localPlayer->movement_flags & MOVEFLAG_SWIMMING) ||
            CurrentGoal.TargetSwimming != targetSwimming ||
            CurrentGoal.Goal.DistanceTo2D(goal) > (type == MoveTypes::Follow ? 24.0f : 2.5f))
            return false;

        Position waypoint;
        if (Navigation::GetCurrentMovementDestination(waypoint) &&
            localPlayer->position.DistanceTo2D(waypoint) > 1.5f) {
            if (Navigation::IsCurrentMovementUnsafe(mapID, localPlayer->position) ||
                Navigation::IsCurrentMovementStalled(localPlayer->position)) {
                Functions::StopMovement();
                return false;
            }
            Moving = type;
            return true;
        }

        if (TryServerWaypoint(goal, avoidEnemies, type == MoveTypes::Follow, ignoredGuid,
                type == MoveTypes::GoToTarget || type == MoveTypes::MoveLoS || type == MoveTypes::MoveLoSAoE)) {
            Moving = type;
            RememberGoal(goal, type, targetSwimming, waterOnRoute, waterEntry);
            return true;
        }
        Navigation::ClearMovement();
        return false;
    }

    void resetMovement() { Functions::StopMovement(); }

    struct LateralRecoveryState {
        bool Active = false;
        int MapId = -1;
        MoveTypes Type = MoveTypes::NotMoving;
        int LastSide = 0;
        Position Destination{};
        Position LastProgressPosition{};
        std::chrono::steady_clock::time_point LastProgressAt{};
        std::chrono::steady_clock::time_point Expires{};
        ActiveGoal ResumeDetour{};
    };
    LateralRecoveryState LateralRecovery;

    Position BuildSwimStep(const Position& from, const Position& target, float cosDir, float sinDir, float step) {
        const float dz = std::clamp(target.Z - from.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
        return Position(from.X + cosDir * step, from.Y + sinDir * step, from.Z + dz);
    }

    bool TryLocalRecoveryStep(const Position& destination, bool checkEnemyClose,
        bool allowPartial = false, unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
        if (Navigation::IsPositionDangerous(zones, destination) ||
            start.DistanceTo2D(destination) < MIN_MOVE_DISTANCE) return false;

        const float base = std::atan2(destination.Y - start.Y, destination.X - start.X);
        const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
        for (const float stride : {3.0f, 5.0f, 8.0f}) {
            std::vector<std::pair<float, Position>> candidates;
            candidates.reserve(MOVE_OFFSETS.size());
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
                    const auto water = WaterMap::At(mapID, next.X, next.Y, next.Z);
                    if (!water.CanSwim()) continue;
                    next.Z = std::clamp(water.surface - 0.6f,
                        start.Z - SWIM_VERTICAL_STEP, start.Z + SWIM_VERTICAL_STEP);
                    if (next.Z < water.ground + 0.5f || next.Z >= water.surface ||
                        Functions::Intersect(start, next, 1.25f)) continue;
                }
                else if (!ProjectSafeGroundStep(start, wanted, next, 1.25f, zones, false)) continue;

                if (!Navigation::IsSegmentSafe(zones, start, next) ||
                    IsRecentlyFailedHeading(start, next) ||
                    start.DistanceTo(next) < MIN_MOVE_DISTANCE ||
                    (checkEnemyClose && !IsPathSafeFromEnemies(start, next, ignoredGuid))) continue;
                const float score = next.DistanceTo2D(destination) +
                    std::abs(off) * 0.08f + stride * 0.08f;
                candidates.emplace_back(score, next);
            }
            std::sort(candidates.begin(), candidates.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            for (const auto& candidate : candidates) {
                if (!swimming) {
                    const bool partialRecovery = allowPartial &&
                        start.DistanceTo2D(destination) > 30.0f &&
                        candidate.second.DistanceTo2D(destination) + MIN_MOVE_DISTANCE <
                            start.DistanceTo2D(destination);
                    if (partialRecovery) {
                        if (!HasSafeGroundContinuation(candidate.second, destination, zones) ||
                            !HasDirectGroundRoute(start, candidate.second, zones)) continue;
                    } else if (!HasValidatedPivot(start, candidate.second, destination, zones)) {
                        continue;
                    }
                }
                if (SafeClickToMove(candidate.second, localPlayer->Guid)) return true;
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
        if (!allowPartial && !findLineOfSight && start.DistanceTo(goal) > 40.0f)
            return false;
        if (!findLineOfSight) {
            if (distance >= MIN_MOVE_DISTANCE && distance <= (allowPartial ? partialHorizon : 30.0f) &&
                HasDirectGroundRoute(start, goal, zones) &&
                (!avoidEnemies || IsPathSafeFromEnemies(start, goal, ignoredGuid)))
                return SafeClickToMove(goal, localPlayer->Guid);

            if (allowPartial && distance > partialHorizon) {
                const float t = partialHorizon / distance;
                const Position horizon(start.X + (goal.X - start.X) * t,
                    start.Y + (goal.Y - start.Y) * t,
                    start.Z + (goal.Z - start.Z) * t);
                if (HasDirectGroundRoute(start, horizon, zones) &&
                    !IsRecentlyFailedHeading(start, horizon) &&
                    (!avoidEnemies || IsPathSafeFromEnemies(start, horizon, ignoredGuid)) &&
                    SafeClickToMove(horizon, localPlayer->Guid)) return true;
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
                            SafeClickToMove(candidate, localPlayer->Guid)) return true;
                        break;
                    }
                } else if (i >= 2 && i % 3 == 2 &&
                           start.DistanceTo2D(candidate) >= MIN_MOVE_DISTANCE &&
                           !IsRecentlyFailedHeading(start, candidate) &&
                           candidate.DistanceTo2D(goal) <= 30.0f &&
                           HasValidatedPivot(start, candidate, goal, zones) &&
                           (!avoidEnemies || IsPathSafeFromEnemies(start, candidate, ignoredGuid)) &&
                           SafeClickToMove(candidate, localPlayer->Guid)) {
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
                    SafeClickToMove(candidate.second, localPlayer->Guid)) return true;
            }
        }
        return false;
    }

    bool TrySwimMove(const Position& goal, bool avoidEnemies, bool findLineOfSight,
        unsigned long long ignoredGuid = 0) {
        const Position start = localPlayer->position;
        const float distance = start.DistanceTo2D(goal);
        if (distance < MIN_MOVE_DISTANCE) {
            if (std::abs(goal.Z - start.Z) > 0.75f) {
                const Position vertical(start.X, start.Y,
                    start.Z + std::clamp(goal.Z - start.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP));
                return !Functions::Intersect(start, vertical, 1.25f) &&
                    (!avoidEnemies || IsPathSafeFromEnemies(start, vertical, ignoredGuid)) &&
                    SafeClickToMove(vertical, localPlayer->Guid);
            }
            return findLineOfSight && !Functions::Intersect(start, goal);
        }

        const float step = (std::min)(distance, SWIM_STEP);
        const float base = std::atan2(goal.Y - start.Y, goal.X - start.X);
        const auto targetWater = WaterMap::At(mapID, goal.X, goal.Y, goal.Z);
        const bool targetInWater = targetWater.CanSwim();

        if (distance > SWIM_STEP + 0.5f) {
            const float longStep = (std::min)(distance, 14.0f);
            float validStep = 0.0f;
            const int samples = static_cast<int>(std::ceil(longStep / 2.0f));
            for (int i = 1; i <= samples; ++i) {
                const float travelled = (std::min)(longStep, i * 2.0f);
                const float fraction = travelled / distance;
                const auto water = WaterMap::At(mapID,
                    start.X + (goal.X - start.X) * fraction,
                    start.Y + (goal.Y - start.Y) * fraction, start.Z);
                if (water.kind == WaterMap::Kind::Hazard ||
                    (water.Known() && !water.CanSwim()) ||
                    (!water.Known() && travelled > 6.0f)) break;
                validStep = travelled;
            }
            if (validStep > SWIM_STEP + 0.5f) {
                const float t = validStep / distance;
                Position direct(start.X + (goal.X - start.X) * t,
                    start.Y + (goal.Y - start.Y) * t, start.Z);
                const auto destinationWater = WaterMap::At(mapID, direct.X, direct.Y, direct.Z);
                bool validHeight = true;
                if (destinationWater.CanSwim()) {
                    const bool submergedTarget = targetInWater && goal.Z < targetWater.surface - 2.0f;
                    const float desiredZ = submergedTarget ? goal.Z : destinationWater.surface - 0.6f;
                    direct.Z = std::clamp(desiredZ,
                        start.Z - SWIM_VERTICAL_STEP, start.Z + SWIM_VERTICAL_STEP);
                    validHeight = direct.Z >= destinationWater.ground + 0.5f &&
                        direct.Z < destinationWater.surface;
                }
                if (validHeight &&
                    !Functions::Intersect(start, direct, 1.25f) &&
                    (!avoidEnemies || IsPathSafeFromEnemies(start, direct, ignoredGuid)) &&
                    (!findLineOfSight || !Functions::Intersect(direct, goal)) &&
                    SafeClickToMove(direct, localPlayer->Guid)) return true;
            }
        }

        Position fallback{};
        bool hasFallback = false;
        for (int offset : MOVE_OFFSETS) {
            const float dir = base + offset * ANGLE_STEP;
            Position candidate = BuildSwimStep(start, goal, std::cos(dir), std::sin(dir), step);
            const auto water = WaterMap::At(mapID, candidate.X, candidate.Y, candidate.Z);
            if (water.kind == WaterMap::Kind::Hazard ||
                (water.Known() && !water.CanSwim() && targetInWater)) continue;
            if (water.CanSwim()) {
                const bool submergedTarget = targetInWater && goal.Z < targetWater.surface - 2.0f;
                const float desiredZ = submergedTarget ? goal.Z : water.surface - 0.6f;
                candidate.Z = std::clamp(desiredZ,
                    start.Z - SWIM_VERTICAL_STEP, start.Z + SWIM_VERTICAL_STEP);
                if (candidate.Z < water.ground + 0.5f || candidate.Z >= water.surface) continue;
            } else if (water.kind == WaterMap::Kind::Dry ||
                       water.kind == WaterMap::Kind::Shallow) {
                const Position projected = Functions::ProjectPos(candidate);
                if (std::abs(projected.Z - candidate.Z) > 1.5f) continue;
                candidate = projected;
            }
            if (start.DistanceTo(candidate) < MIN_MOVE_DISTANCE ||
                Functions::Intersect(start, candidate, 1.25f) ||
                (avoidEnemies && !IsPathSafeFromEnemies(start, candidate, ignoredGuid))) continue;

            if (!findLineOfSight || !Functions::Intersect(candidate, goal)) {
                if (SafeClickToMove(candidate, localPlayer->Guid)) return true;
            } else if (!hasFallback) {
                fallback = candidate;
                hasFallback = true;
            }
        }
        return findLineOfSight && hasFallback && SafeClickToMove(fallback, localPlayer->Guid);
    }

    bool TryWaterEntryMove(const Position& goal, bool avoidEnemies, bool targetSwimming = false,
        unsigned long long ignoredGuid = 0, const Position* retreatFrom = nullptr) {
        const Position start = localPlayer->position;
        const float distance = start.DistanceTo2D(goal);
        if (distance < MIN_MOVE_DISTANCE) return false;

        const auto currentWater = WaterMap::At(mapID, start.X, start.Y, start.Z);
        const auto immersion = currentWater.RelativeTo(start.Z);
        const bool alreadyInWater = immersion == WaterMap::Level::In ||
            immersion == WaterMap::Level::Under;
        const float step = (std::min)(distance, SWIM_STEP);
        const float base = std::atan2(goal.Y - start.Y, goal.X - start.X);
        bool longRayChecked = false;
        bool clearToSwimmer = false;
        for (int offset : MOVE_OFFSETS) {
            if (std::abs(offset) > 4) continue;
            const float angle = base + offset * ANGLE_STEP;
            const Position wanted(start.X + std::cos(angle) * step,
                start.Y + std::sin(angle) * step, start.Z);
            const auto water = WaterMap::At(mapID, wanted.X, wanted.Y, wanted.Z);
            if (water.kind == WaterMap::Kind::Hazard) continue;
            Position next = wanted;
            if (water.CanSwim()) {
                if (!alreadyInWater && start.Z > water.surface + 1.5f) continue;
                const float desiredZ = alreadyInWater ?
                    std::clamp(goal.Z, water.ground + 0.6f,
                        (std::max)(water.ground + 0.6f, water.surface - 0.35f)) :
                    water.surface - 0.6f;
                next.Z = std::clamp(desiredZ,
                    start.Z - SWIM_VERTICAL_STEP, start.Z + SWIM_VERTICAL_STEP);
            } else {
                const Position projected = Functions::ProjectPos(wanted);
                if (projected.DistanceTo(wanted) < 2.0f) {
                    next = projected;
                } else {
                    if ((water.Known() && !targetSwimming) || std::abs(offset) > 2) continue;
                    if (!longRayChecked) {
                        clearToSwimmer = !Functions::Intersect(start, goal, 1.25f);
                        longRayChecked = true;
                    }
                    if (!clearToSwimmer) continue;
                    next.Z += std::clamp(goal.Z - start.Z,
                        -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
                }
            }

            if (next.DistanceTo2D(goal) >= distance - 0.25f ||
                start.DistanceTo(next) < MIN_MOVE_DISTANCE ||
                (retreatFrom && next.DistanceTo2D(*retreatFrom) <=
                    start.DistanceTo2D(*retreatFrom) + 0.25f) ||
                Functions::Intersect(start, next, 1.25f) ||
                (retreatFrom && Functions::Intersect(next, *retreatFrom)) ||
                (avoidEnemies && !IsPathSafeFromEnemies(start, next, ignoredGuid))) continue;
            if (SafeClickToMove(next, localPlayer->Guid)) return true;
        }
        return false;
    }

    bool ChooseMeleeApproach(Position& target_pos, bool meleeTarget, bool checkEnemyClose,
        const Navigation::AvoidanceSnapshot& zones) {
        if (Navigation::IsPositionDangerous(zones, target_pos) && meleeTarget) {
            const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
            const float range = localPlayer->combatReach + targetUnit->combatReach + 1.0f;
            const bool previousValid = LastMeleeApproach.Valid && LastMeleeApproach.Map == mapID &&
                LastMeleeApproach.Guid == targetUnit->Guid &&
                LastMeleeApproach.Swimming == swimming &&
                LastMeleeApproach.Boss.DistanceTo(target_pos) < 1.5f &&
                LastMeleeApproach.Goal.DistanceTo(target_pos) <=
                localPlayer->combatReach + targetUnit->combatReach + 1.5f &&
                !Navigation::IsPositionDangerous(zones, LastMeleeApproach.Goal) &&
                !Functions::Intersect(LastMeleeApproach.Goal, target_pos);
            if (previousValid) {
                target_pos = LastMeleeApproach.Goal;
            }
            else {
                bool found = false;
                Position best;
                const float base = std::atan2(localPlayer->position.Y - target_pos.Y,
                    localPlayer->position.X - target_pos.X);
                std::array<std::pair<float, Position>, 24> positions{};
                for (int i = 0; i < 24; ++i) {
                    const float angle = base + i * (2.0f * PI / 24.0f);
                    const Position desired(target_pos.X + std::cos(angle) * range,
                        target_pos.Y + std::sin(angle) * range, target_pos.Z);
                    positions[i] = { localPlayer->position.DistanceTo2D(desired), desired };
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
                        if (!water.CanSwim()) continue;
                        candidate.Z = std::clamp(target_pos.Z, water.ground + 0.6f,
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
                if (!found) {
                    LastMeleeApproach.Valid = false;
                    return false;
                }
                LastMeleeApproach = { targetUnit->Guid, mapID, target_pos, best,
                    swimming, true };
                target_pos = best;
            }
        }
        else {
            LastMeleeApproach.Valid = false;
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
            const bool sameDetour = LastCombatDetour.Map == mapID &&
                LastCombatDetour.Guid == blocking->Guid &&
                LastCombatDetour.Center.DistanceTo2D(blocking->Center) < 3.0f &&
                LastCombatDetour.Goal.DistanceTo2D(target_pos) < 3.0f;
            const int side = sameDetour ? LastCombatDetour.Side :
                (std::abs(angleDifference) < 0.05f ?
                    (localPlayer->Guid % 2ULL ? 1 : -1) :
                    (angleDifference > 0.0f ? 1 : -1));
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
                    if (SafeClickToMove(candidate, localPlayer->Guid)) {
                        LastCombatDetour = { blocking->Guid, mapID, direction,
                                            blocking->Center, target_pos };
                        return true;
                    }
                }
            }
        }
        return false;
    }
}

bool Functions::StartMovement(const Position& destination, bool emergency,
    bool navigationWaypoint, unsigned long long guid) {
    if (!localPlayer) return false;
    const Position start = localPlayer->position;
    if (!std::isfinite(destination.X) || !std::isfinite(destination.Y) || !std::isfinite(destination.Z)) {
        return false;
    }
    if (!emergency && IsRecentlyFailedHeading(start, destination)) {
        bool rejected = !navigationWaypoint;
        if (navigationWaypoint) {
            for (const auto& failed : FailedHeadings) {
                if (failed.MapId == mapID && start.DistanceTo2D(failed.Origin) <= 5.0f &&
                    destination.DistanceTo2D(failed.Destination) <= 2.0f) {
                    rejected = true;
                    break;
                }
            }
        }
        if (rejected) {
            return false;
        }
    }
    if (!Navigation::IsSegmentSafe(mapID, start, destination)) {
        return false;
    }
    localPlayer->ClickToMove(Move, guid ? guid : localPlayer->Guid, destination);
    CurrentGoal.NavigationWaypoint = navigationWaypoint;
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
    CurrentGoal.Valid = false;
    CurrentGoal.NavigationWaypoint = false;
    LateralRecovery.Active = false;
    LateralRecovery.ResumeDetour = {};
    Navigation::ClearMovement();
    Moving = MoveTypes::NotMoving;
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
    if (found && SafeClickToMove(best, localPlayer->Guid, true)) {
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
        const ActiveGoal resumeDetour = LateralRecovery.ResumeDetour;
        Functions::StopMovement();
        if (resumeDetour.Valid && resumeDetour.NavigationWaypoint &&
            resumeDetour.Map == mapID)
            CurrentGoal = resumeDetour;
        return false;
    }
    return true;
}

void Functions::HandleMovementStall() {
    if (LateralRecovery.Active || localPlayer == nullptr) return;
    Position failedDestination;
    const MoveTypes previousType = Moving;
    const ActiveGoal previousGoal = CurrentGoal;
    const bool hasFailedWaypoint = Navigation::GetCurrentMovementDestination(failedDestination) &&
        localPlayer->position.DistanceTo2D(failedDestination) >= MIN_MOVE_DISTANCE;
    if (hasFailedWaypoint) {
        FailedHeadings.push_back({ localPlayer->position, failedDestination, mapID,
                                  std::chrono::steady_clock::now() });
        if (FailedHeadings.size() > 8) FailedHeadings.erase(FailedHeadings.begin());
    }
    resetMovement();
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
            if (!safe) continue;
            if (IsRecentlyFailedHeading(start, candidate) ||
                !SafeClickToMove(candidate, localPlayer->Guid)) continue;
            LateralRecovery.Active = true;
            LateralRecovery.MapId = mapID;
            LateralRecovery.Type = previousType;
            LateralRecovery.LastSide = side;
            LateralRecovery.Destination = candidate;
            LateralRecovery.LastProgressPosition = start;
            LateralRecovery.LastProgressAt = std::chrono::steady_clock::now();
            LateralRecovery.Expires = LateralRecovery.LastProgressAt + std::chrono::milliseconds(1350);
            if (previousGoal.Valid && previousGoal.NavigationWaypoint &&
                previousGoal.Map == mapID && previousGoal.Type == previousType)
                LateralRecovery.ResumeDetour = previousGoal;
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
    if (start.DistanceTo2D(target->position) >= needed) return true;
    Position waypoint;
    if (Moving == moveType && Navigation::GetCurrentMovementDestination(waypoint) &&
        !Navigation::IsCurrentMovementStalled(start) &&
        !Navigation::IsCurrentMovementUnsafe(mapID, start) &&
        waypoint.DistanceTo2D(target->position) >= needed &&
        IsPathSafeFromEnemies(start, waypoint, target->Guid) &&
        !Functions::Intersect(waypoint, target->position)) return true;

    const float angle = start.DistanceTo2D(target->position) > 0.01f ?
        std::atan2(start.Y - target->position.Y, start.X - target->position.X) :
        localPlayer->facing + PI;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    struct Retreat { float score; Position point; };
    std::vector<Retreat> candidates;
    candidates.reserve(52);
    for (float stride : { 6.0f, 12.0f, 18.0f, 24.0f }) {
        for (int offset : MOVE_OFFSETS) {
            const float dir = angle + offset * ANGLE_STEP;
            const Position wanted(start.X + std::cos(dir) * stride,
                start.Y + std::sin(dir) * stride, start.Z);
            if (wanted.DistanceTo2D(target->position) < needed ||
                wanted.DistanceTo2D(target->position) <= start.DistanceTo2D(target->position) ||
                Navigation::IsPositionDangerous(zones, wanted) ||
                !Navigation::IsSegmentSafe(zones, start, wanted) ||
                IsRecentlyFailedHeading(start, wanted)) continue;
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
                if (std::abs(projected.Z - chosen.Z) > 1.5f) continue;
                chosen = projected;
            }
            if (Functions::Intersect(start, chosen, 1.25f)) continue;
        } else {
            Position projected = Functions::ProjectPos(chosen);
            if (projected.DistanceTo(chosen) >= 2.0f ||
                !HasDirectGroundRoute(start, projected, zones)) continue;
            chosen = projected;
        }
        if (IsPathSafeFromEnemies(start, chosen, target->Guid) &&
            !Functions::Intersect(chosen, target->position) &&
            SafeClickToMove(chosen, localPlayer->Guid)) {
            Moving = moveType;
            RememberGoal(chosen, moveType);
            return true;
        }
    }
    if (!swimming) {
        int checked = 0;
        for (const auto& candidate : candidates) {
            if (++checked > 10) break;
            const auto water = InspectWaterCorridor(start, candidate.point);
            if (!water.crosses || !water.entryNear) continue;
            if (TryWaterEntryMove(candidate.point, true, false, target->Guid,
                &target->position)) {
                Moving = moveType;
                RememberGoal(candidate.point, moveType, false, water.crosses, water.entryNear);
                return true;
            }
        }
    }
    resetMovement();
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
    if (!localPlayer) return;
    if (LateralRecovery.Active) {
        if (moveType != MoveTypes::BossMechanic) return;
        LateralRecovery.Active = false;
    }
    const Position start = localPlayer->position;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    if (start.DistanceTo(target_pos) < MIN_MOVE_DISTANCE) {
        if (Moving == moveType) StopMovement();
        return;
    }
    const auto water = swimming ? WaterCorridor{} : InspectWaterCorridor(start, target_pos);
    if (ReuseActiveWaypoint(target_pos, moveType, false, water.crosses, water.entryNear)) return;
    if (ContinueDetour(target_pos, moveType, true, false, water.crosses, water.entryNear)) return;
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
        if (!started)
            started = TryServerWaypoint(target_pos, true);
    }
    if (started) {
        Moving = moveType;
        RememberGoal(target_pos, moveType, false, water.crosses, water.entryNear);
    } else {
        StopMovement();
    }
}

void Functions::MoveTo(Position target_pos, MoveTypes moveType, bool checkEnemyClose, bool targetSwim) {
    if (!localPlayer) return;
    if (LateralRecovery.Active) {
        if (moveType != MoveTypes::BossMechanic) return;
        LateralRecovery.Active = false;
    }
    const auto zones = Navigation::GetAvoidanceSnapshot(mapID);
    const bool meleeTarget = moveType == MoveTypes::GoToTarget && targetUnit &&
        targetUnit->attackable && !targetUnit->isdead &&
        target_pos.DistanceTo2D(targetUnit->position) < 2.0f &&
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

    if (ReuseActiveWaypoint(target_pos, moveType, targetSwim, water.crosses, water.entryNear)) return;
    if (ContinueDetour(target_pos, moveType, checkEnemyClose, targetSwim, water.crosses, water.entryNear, ignoredGuid)) return;

    if (swimming) {
        if (TrySwimMove(target_pos, checkEnemyClose, false, ignoredGuid) ||
            TryLocalRecoveryStep(target_pos, checkEnemyClose, follow, ignoredGuid)) {
            Moving = moveType;
            RememberGoal(target_pos, moveType, targetSwim, water.crosses, water.entryNear);
        }
        else if (!follow || !ReuseActiveWaypoint(target_pos, moveType, targetSwim, water.crosses, water.entryNear)) {
            StopMovement();
        }
        return;
    }

    if (water.entryNear || (targetSwim && distanceToGoal <= 5.0f)) {
        bool started = TryWaterEntryMove(target_pos, checkEnemyClose, targetSwim, ignoredGuid);
        if (!started && water.crosses)
            started = TryGroundMove(target_pos, checkEnemyClose, false, true,
                water.groundHorizon, ignoredGuid);
        if (!started)
            started = TryServerWaypoint(target_pos, checkEnemyClose, follow, ignoredGuid,
                moveType == MoveTypes::GoToTarget);
        if (!started)
            started = TryLocalRecoveryStep(target_pos, checkEnemyClose, follow, ignoredGuid);
        if (started) {
            Moving = moveType;
            RememberGoal(target_pos, moveType, targetSwim, water.crosses, water.entryNear);
        } else if (!follow || !ReuseActiveWaypoint(target_pos, moveType, targetSwim, water.crosses, water.entryNear)) {
            StopMovement();
        }
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
    if (serverFirst) started = TryServerWaypoint(target_pos, checkEnemyClose,
        follow, ignoredGuid, moveType == MoveTypes::GoToTarget);
    if (!started) started = TryGroundMove(target_pos, checkEnemyClose, false,
        (follow || water.crosses) && !steepHeightChange && !failedHeading,
        water.groundHorizon, ignoredGuid);
    if (!started && targetSwim && distanceToGoal <= 5.0f)
        started = TryWaterEntryMove(target_pos, checkEnemyClose, targetSwim, ignoredGuid);
    if (!started && !serverFirst)
        started = TryServerWaypoint(target_pos, checkEnemyClose, follow, ignoredGuid,
            moveType == MoveTypes::GoToTarget);

    if (!started && meleeTarget && !Navigation::IsSegmentSafe(zones, start, target_pos))
        started = TryCombatDetour(target_pos, checkEnemyClose, zones);

    if (!started) started = TryLocalRecoveryStep(target_pos, checkEnemyClose, follow, ignoredGuid);
    if (started) {
        Moving = moveType;
        RememberGoal(target_pos, moveType, targetSwim, water.crosses, water.entryNear);
    }
    else if (!follow || !ReuseActiveWaypoint(target_pos, moveType, targetSwim, water.crosses, water.entryNear)) {
        StopMovement();
    }
}

void Functions::MoveToLoS(Position target_pos, MoveTypes moveType, bool targetSwim) {
    if (!localPlayer) return;
    if (LateralRecovery.Active) {
        if (moveType != MoveTypes::BossMechanic) return;
        LateralRecovery.Active = false;
    }
    const Position start = localPlayer->position;
    const bool swimming = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) != 0;
    const auto water = swimming ? WaterCorridor{} : InspectWaterCorridor(start, target_pos);
    const unsigned long long ignoredGuid = targetUnit &&
        target_pos.DistanceTo2D(targetUnit->position) < 2.0f ? targetUnit->Guid : 0;
    if (ReuseActiveWaypoint(target_pos, moveType, targetSwim, water.crosses, water.entryNear)) return;
    if (ContinueDetour(target_pos, moveType, true, targetSwim, water.crosses, water.entryNear, ignoredGuid)) return;

    bool started = false;
    if (swimming) {
        started = TrySwimMove(target_pos, true, true, ignoredGuid);
    } else {
        started = TryGroundMove(target_pos, true, true, false, 16.0f, ignoredGuid);
        if (!started && (water.entryNear ||
            (targetSwim && start.DistanceTo2D(target_pos) <= 5.0f)))
            started = TryWaterEntryMove(target_pos, true, targetSwim, ignoredGuid);
        if (!started && water.crosses && !water.entryNear)
            started = TryGroundMove(target_pos, true, false, true, water.groundHorizon, ignoredGuid);
        if (!started)
            started = TryServerWaypoint(target_pos, true, false, ignoredGuid,
                moveType == MoveTypes::MoveLoS || moveType == MoveTypes::MoveLoSAoE);
    }
    if (!started)
        started = TryLocalRecoveryStep(target_pos, true, false, ignoredGuid);
    if (started) {
        Moving = moveType;
        RememberGoal(target_pos, moveType, targetSwim, water.crosses, water.entryNear);
    } else StopMovement();
}
