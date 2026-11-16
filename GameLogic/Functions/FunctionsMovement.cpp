#define NOMINMAX
#include "Functions.h"
#include "../MemoryManager.h"
#include "../Game.h"
#include "../Navigation.h"
#include "../WaterMap.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float STEP = 2.0f;
    constexpr int MAX_STEPS = 20;
    constexpr int OFFSETS[] = { 0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6, 7, -7, 8 };

    WaterMap::Sample WaterAt(const Position& point) {
        return WaterMap::At(mapID, point.X, point.Y, point.Z);
    }

    bool IsWater(const WaterMap::Sample& water) {
        return water.CanSwim() || water.kind == WaterMap::Kind::Shallow;
    }

    bool ClearMove(const Position& start, const Position& goal) {
        return Navigation::IsSegmentSafe(mapID, start, goal) && !Functions::Intersect(start, goal);
    }

    void ClickMove(const Position& point, MoveTypes type) {
        localPlayer->ClickToMove(Move, localPlayer->Guid, point);
        Moving = type;
    }

    bool IsGroundSafe8Points(const Position& center, float radius = 2.0f) {
        for (int i = 0; i < 8; ++i) {
            const float angle = i * PI / 4.0f;
            const Position probe(center.X + std::cos(angle) * radius, center.Y + std::sin(angle) * radius, center.Z);
            const auto water = WaterAt(probe);
            if (water.kind == WaterMap::Kind::Hazard) return false;
            if (IsWater(water)) continue; // L'eau est praticable, inutile de sonder son fond.
            const Position ground = Functions::ProjectPos(probe);
            if (std::abs(ground.Z - center.Z) >= STEP || Functions::Intersect(ground, center, 1.0f)) return false;
        }
        return true;
    }


    bool canMoveStraightLine(const Position& start, const Position& goal, Position& nextPoint, bool checkEnemies = true) {
        const float distance = start.DistanceTo2D(goal);
        if (distance > STEP * MAX_STEPS) return false;
        if (distance < 0.001f) {
            nextPoint = goal;
            return ClearMove(start, goal) && (!checkEnemies || !Functions::enemyClose(goal));
        }

        const int steps = static_cast<int>(std::ceil(distance / STEP));
        Position last = start;
        bool previousWater = IsWater(WaterAt(start));
        bool found = false, visible = true;

        for (int i = 1; i <= steps; ++i) {
            const float t = (std::min)(i * STEP / distance, 1.0f);
            Position point(start.X + (goal.X - start.X) * t, start.Y + (goal.Y - start.Y) * t, start.Z + (goal.Z - start.Z) * t);

            const auto water = WaterAt(point);
            if (water.kind == WaterMap::Kind::Hazard) return false;
            const bool aquatic = IsWater(water);

            if (i < steps) {
                if (aquatic) {
                    if (!std::isfinite(water.surface)) return false;
                    point.Z = water.surface;
                }
                else point = Functions::ProjectPos(point);
            }

            if (!aquatic && !previousWater && (std::abs(point.Z - last.Z) >= STEP || !IsGroundSafe8Points(point))) return false;
            if (!ClearMove(last, point) || (checkEnemies && Functions::enemyClose(point))) return false;

            if (visible) {
                if (ClearMove(start, point)) { nextPoint = point; found = true; }
                else visible = false;
            }

            last = point;
            previousWater = aquatic;
        }

        return found;
    }

    bool MoveObstacleSwim(const Position& start, const Position& goal, MoveTypes moveType) {
        const float base = std::atan2(goal.Y - start.Y, goal.X - start.X);
        for (int step = 1; step <= MAX_STEPS; ++step) {
            for (int offset : OFFSETS) {
                const float radius = step * STEP;
                const float angle = base + offset * PI / 8.0f;
                const Position point(start.X + std::cos(angle) * radius, start.Y + std::sin(angle) * radius, start.Z);
                if (!IsWater(WaterAt(point))) continue;
                if (!ClearMove(start, point) || !ClearMove(point, goal) || Functions::enemyClose(point)) continue;
                ClickMove(point, moveType);
                return true;
            }
        }
        return false;
    }
}

void Functions::StopMovement() {
    if (!localPlayer) { Moving = MoveTypes::NotMoving; return; }
    if (localPlayer->movement_flags & MOVEFLAG_BACKWARD) ThreadSynchronizer::releaseKey(0x28);
    else if (localPlayer->isMoving || (localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
        ThreadSynchronizer::pressKey(0x28);
        ThreadSynchronizer::releaseKey(0x28);
    }
    Moving = MoveTypes::NotMoving;
}

void Functions::FollowMultibox(int placement) {
    if (!localPlayer || !Leader) return;
    float range = 2.0f, offset = 0.30f;
    if (placement == 2) offset = -0.30f;
    else if (placement == 3 || placement == 4) {
        range = 4.0f;
        offset = (placement == 3 ? 1.0f : -1.0f) * (localPlayer->isMounted ? 0.45f : 0.15f);
    }
    const float angle = Leader->facing + PI + offset;
    const Position goal(Leader->position.X + std::cos(angle) * range, Leader->position.Y + std::sin(angle) * range, Leader->position.Z);
    if (localPlayer->position.DistanceTo(goal) <= STEP) {
        if (Moving == MoveTypes::Follow) Functions::StopMovement();
        return;
    }
    ThreadSynchronizer::RunOnMainThread([goal]() { Functions::MoveTo(goal, MoveTypes::Follow); });
}

void Functions::MoveTo(Position goal, MoveTypes moveType) {
    if (!localPlayer) return;
    const Position start = localPlayer->position;
    const auto waterStart = WaterAt(start);
    const auto waterGoal = WaterAt(goal);
    if (waterGoal.kind == WaterMap::Kind::Hazard) return;

    const bool inWater = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) || IsWater(waterStart);
    const bool aquaticGoal = IsWater(waterGoal);

    if (inWater && aquaticGoal) {
        if (ClearMove(start, goal)) ClickMove(goal, moveType);
        else MoveObstacleSwim(start, goal, moveType);
        return;
    }

    if ((inWater || aquaticGoal) && ClearMove(start, goal)) { ClickMove(goal, moveType); return; }

    Position next;
    if (canMoveStraightLine(start, goal, next)) { ClickMove(next, moveType); return; }

    else if (inWater) { MoveObstacleSwim(start, goal, moveType); return; }
    else if (aquaticGoal) return;

    else {
        Position goalProj = Functions::ProjectPos(goal);
        const Position path = Navigation::CalculatePath(mapID, start, goalProj);
        if (path.DistanceTo(start) >= STEP && WaterAt(path).kind != WaterMap::Kind::Hazard && !Functions::enemyClose(path)) ClickMove(path, moveType);
    }
}

bool Functions::StepBack(WoWUnit* target, MoveTypes moveType, float dist_away) {
    if (!target || !localPlayer) return false;
    const Position start = localPlayer->position, enemy = target->position;
    const bool swimming = localPlayer->movement_flags & MOVEFLAG_SWIMMING;
    const bool inWater = swimming || IsWater(WaterAt(start));
    const float needed = dist_away + localPlayer->combatReach + target->combatReach;
    const auto distance = [&](const Position& p) { return swimming ? p.DistanceTo(enemy) : p.DistanceTo2D(enemy); };
    if (distance(start) >= needed) return true;

    const float base = start.DistanceTo2D(enemy) > 0.01f ? std::atan2(start.Y - enemy.Y, start.X - enemy.X) : localPlayer->facing + PI;
    for (int step = 1; step <= MAX_STEPS; ++step) {
        for (int offset : OFFSETS) {
            const float radius = step * STEP;
            const float angle = base + offset * PI / 8.0f;
            Position point(start.X + std::cos(angle) * radius, start.Y + std::sin(angle) * radius, start.Z), next = point;
            const auto water = WaterAt(point);
            if (water.kind == WaterMap::Kind::Hazard) continue;
            if (!IsWater(water)) point = Functions::ProjectPos(point);
            else if (!inWater) point.Z = water.surface;
            if (distance(point) < needed || Functions::Intersect(point, enemy)) continue;

            if (inWater && IsWater(water)) {
                if (!ClearMove(start, point)) continue;
            }
            else if (!canMoveStraightLine(start, point, next, false)) continue; // Ne pas bloquer sur l'ennemi qu'on fuit.

            ClickMove(next, moveType);
            return true;
        }
    }
    return false;
}

void Functions::MoveToLoS(Position target_pos, MoveTypes moveType) {
    if (!localPlayer) return;
    const Position start = localPlayer->position;
    if (!Functions::Intersect(start, target_pos)) return;
    const bool inWater = (localPlayer->movement_flags & MOVEFLAG_SWIMMING) || IsWater(WaterAt(start));
    const float base = std::atan2(target_pos.Y - start.Y, target_pos.X - start.X);

    for (int step = 1; step <= MAX_STEPS; ++step) {
        for (int offset : OFFSETS) {
            const float angle = base + offset * PI / 4.0f, radius = step * STEP;
            Position point(start.X + std::cos(angle) * radius, start.Y + std::sin(angle) * radius, start.Z), next = point;
            const auto water = WaterAt(point);
            if (water.kind == WaterMap::Kind::Hazard) continue;
            if (!IsWater(water)) point = Functions::ProjectPos(point);
            else if (!inWater) point.Z = water.surface;
            if (Functions::Intersect(point, target_pos) || Functions::enemyClose(point)) continue; // Voir la cible, pas marcher jusqu'à elle.

            if (inWater && IsWater(water)) {
                if (!ClearMove(start, point)) continue;
            }
            else if (!canMoveStraightLine(start, point, next)) continue;

            ClickMove(next, moveType);
            return;
        }
    }
}

bool Functions::AscendForBreath() {
    if (!localPlayer || !(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) return false;
    const Position start = localPlayer->position;
    const auto water = WaterAt(start);
    if (!IsWater(water) || !std::isfinite(water.surface)) return false;
    const Position surface(start.X + std::cos(localPlayer->facing), start.Y + std::sin(localPlayer->facing), water.surface);
    if (start.Z >= surface.Z) return true;
    if (!Navigation::IsSegmentSafe(mapID, start, surface) || Functions::Intersect(start, surface, 0.0f)) return false;
    localPlayer->ClickToMove(Move, localPlayer->Guid, surface);
    return true;
}
