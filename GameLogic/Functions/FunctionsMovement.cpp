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

        for (int i = 0; i < POINT_COUNT; i++) {
            const float angle = static_cast<float>(i) * POINT_ANGLE_STEP;
            Position probe(
                center.X + std::cos(angle) * radius,
                center.Y + std::sin(angle) * radius,
                center.Z
            );

            Position projected = Functions::ProjectPos(probe);
            if (Distance(projected, probe) >= 2.0f || Functions::Intersect(projected, center, 1.0f)) return false;
        }
        return true;
    }

    bool ProjectSafeGroundStep(const Position& from, const Position& wanted, Position& result, float groundCheckRadius) {
        result = Functions::ProjectPos(wanted);
        if (Distance(result, wanted) >= 2.0f) return false;
        if (Functions::Intersect(from, result, 1.0f)) return false;
        return IsGroundSafe8Points(result, groundCheckRadius);
    }

    void resetMovement() {
        if ((localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
            ThreadSynchronizer::pressKey(0x28);
            ThreadSynchronizer::releaseKey(0x28);
            Moving = 0;
        }
        else if (localPlayer->movement_flags & MOVEFLAG_BACKWARD) {
            ThreadSynchronizer::releaseKey(0x28);
            Moving = 0;
        }
    }

    Position BuildSwimStep(const Position& from, const Position& target, float cosDir, float sinDir, float step) {
        const float dz = std::clamp(target.Z - from.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
        return Position(from.X + cosDir * step, from.Y + sinDir * step, from.Z + dz);
    }

    bool TryWaterEntryStep(const Position& target_pos, bool checkEnemyClose) {
        const Position start = localPlayer->position;
        const float targetDist = Distance2D(start, target_pos);
        if (targetDist < MIN_MOVE_DISTANCE) return false;

        const float step = targetDist <= SWIM_STEP ? targetDist : SWIM_STEP;
        const float base = std::atan2(target_pos.Y - start.Y, target_pos.X - start.X);

        for (int off : MOVE_OFFSETS) {
            const float dir = base + off * ANGLE_STEP;
            const float cosDir = std::cos(dir);
            const float sinDir = std::sin(dir);
            Position wanted(start.X + cosDir * step, start.Y + sinDir * step, start.Z);
            Position projected = Functions::ProjectPos(wanted);
            Position next = wanted;

            if (Distance(projected, wanted) < 2.0f) next = projected;
            else {
                const float dz = std::clamp(target_pos.Z - start.Z, -SWIM_VERTICAL_STEP, SWIM_VERTICAL_STEP);
                next.Z = start.Z + dz;
            }

            if (Distance(start, next) < MIN_MOVE_DISTANCE) continue;
            if (Functions::Intersect(start, next, 1.25f)) continue;
            if (checkEnemyClose && Functions::enemyClose(next)) continue;

            localPlayer->ClickToMove(Move, localPlayer->Guid, next);
            return true;
        }

        return false;
    }

    bool MoveObstacleSwimInternal(Position target_pos, bool checkEnemyClose) {
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

            localPlayer->ClickToMove(Move, localPlayer->Guid, next);
            return true;
        }

        return false;
    }
}

bool Functions::StepBack(WoWUnit* target, int move_type, float dist_away) {
    if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) && Moving == move_type) {
        Moving = move_type;
        return true;
    }

    std::vector<Position> list_pos;
    const float halfPI = acosf(0);

    for (int i = 0; i < 32; i++) {
        const float dir = i * halfPI / 8;
        const float stepX = std::cos(dir) * 2.0f;
        const float stepY = std::sin(dir) * 2.0f;
        Position last_pos = target->position;

        for (int w = 0; w < 10; w++) {
            Position tmp_pos(last_pos.X + stepX, last_pos.Y + stepY, last_pos.Z);
            Position next_pos = tmp_pos;

            if (!(localPlayer->movement_flags & MOVEFLAG_SWIMMING)) {
                next_pos = Functions::ProjectPos(tmp_pos);
                if (Distance(next_pos, tmp_pos) >= 2.0f || !IsGroundSafe8Points(next_pos, 3.0f)) break;
            }

            if (!Functions::Intersect(last_pos, next_pos)) {
                if ((target->position.DistanceTo(next_pos) - localPlayer->combatReach - target->combatReach) >= dist_away && !Functions::enemyClose(next_pos) && !Functions::Intersect(next_pos, target->position)) {
                    list_pos.push_back(next_pos);
                    break;
                }

                last_pos = next_pos;
                continue;
            }

            break;
        }
    }

    float min_dist = 99999.0f;
    int min_dist_index = -1;

    for (unsigned int i = 0; i < list_pos.size(); i++) {
        const float dist = list_pos[i].DistanceTo(localPlayer->position);
        if (dist < min_dist) {
            min_dist_index = static_cast<int>(i);
            min_dist = dist;
        }
    }

    if (min_dist_index > -1 && min_dist >= MIN_MOVE_DISTANCE) {
        Position candidate = Functions::RandomisePos(list_pos[min_dist_index], 2.0f, target->position, dist_away + localPlayer->combatReach + target->combatReach);
        if (Distance(candidate, localPlayer->position) < MIN_MOVE_DISTANCE) {
            resetMovement();
            return false;
        }

        localPlayer->ClickToMove(Move, target->Guid, candidate);
        Moving = move_type;
        return true;
    }

    resetMovement();
    return false;
}

// ================== //
// ===== Follow ===== //
// ================== //

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
        ProjectSafeGroundStep(Leader->position, target_pos, projected_pos, 3.0f);
        if (!targetSwim && projected_pos.DistanceTo(target_pos) > (2.0f+range)) {
            resetMovement();
            return;
        }

        Functions::MoveTo(projected_pos, 4, true, targetSwim);
    });
}

void Functions::MoveTo(Position target_pos, int MoveType, bool checkEnemyClose, bool targetSwim) {
    const bool swimming = localPlayer->movement_flags & MOVEFLAG_SWIMMING;

    if (swimming) {
        if (MoveObstacleSwimInternal(target_pos, checkEnemyClose)) Moving = MoveType;
        else resetMovement();
        return;
    }

    if (targetSwim) {
        if (TryWaterEntryStep(target_pos, checkEnemyClose)) Moving = MoveType;
        else resetMovement();
        return;
    }

    if (Navigation::HasBlacklists() || !Functions::MoveObstacle(target_pos, checkEnemyClose)) {
        Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos);

        if (nextpos.DistanceTo(localPlayer->position) >= MIN_MOVE_DISTANCE && !Functions::enemyClose(nextpos)) {
            if ((localPlayer->movement_flags & MOVEFLAG_FORWARD)) return;
            localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
            Moving = MoveType;
        }
        else resetMovement();
    }
    else Moving = MoveType;
}

// =============== //
// ===== LoS ===== //
// =============== //

void Functions::MoveToLoS(Position target_pos, int MoveType) {
    if (localPlayer->movement_flags & MOVEFLAG_SWIMMING) {
        if (Functions::MoveLoSSwim(target_pos)) Moving = MoveType;
        else resetMovement();
        return;
    }

    if (!Functions::MoveLoS(target_pos)) {
        Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, target_pos);

        if (nextpos.DistanceTo(localPlayer->position) >= MIN_MOVE_DISTANCE && !Functions::enemyClose(nextpos)) {
            if ((localPlayer->movement_flags & MOVEFLAG_FORWARD)) return;
            localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
            Moving = MoveType;
        }
        else resetMovement();
    }
    else Moving = MoveType;
}

bool Functions::MoveLoS(Position target_pos) {
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

            if (!ProjectSafeGroundStep(last, stepPos, next, 3.0f)) break;
            if (Functions::enemyClose(next)) break;

            if (!Functions::Intersect(next, target_pos)) {
                if (Distance(localPlayer->position, next) < MIN_MOVE_DISTANCE) break;
                localPlayer->ClickToMove(Move, localPlayer->Guid, next);
                return true;
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
            localPlayer->ClickToMove(Move, localPlayer->Guid, next);
            return true;
        }

        if (!hasFallback) {
            fallback = next;
            hasFallback = true;
        }
    }

    if (!hasFallback) return false;

    localPlayer->ClickToMove(Move, localPlayer->Guid, fallback);
    return true;
}

bool MoveObstacle_tmp(const Position& target_pos, const Position& start_pos) {
    constexpr float STEP = 2.0f;
    constexpr int MAX_STEPS = 15;

    const float dx = target_pos.X - start_pos.X;
    const float dy = target_pos.Y - start_pos.Y;
    const float base = std::atan2(dy, dx);
    const float stepX = std::cos(base) * STEP;
    const float stepY = std::sin(base) * STEP;
    Position last = start_pos;

    for (int i = 0; i < MAX_STEPS; i++) {
        if (last.DistanceTo(target_pos) <= STEP) {
            Position finalPosition;
            return ProjectSafeGroundStep(last, target_pos, finalPosition, 3.0f);
        }

        Position stepPos(last.X + stepX, last.Y + stepY, last.Z);
        Position next;

        if (!ProjectSafeGroundStep(last, stepPos, next, 3.0f)) return false;
        last = next;
    }

    return false;
}

bool Functions::MoveObstacle(Position target_pos, bool checkEnemyClose) {
    if (localPlayer->position.DistanceTo(target_pos) > 40.0f) return false;

    constexpr float STEP = 2.0f;
    constexpr int MAX_STEPS = 15;

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

            if (!ProjectSafeGroundStep(last, stepPos, next, 3.0f)) break;
            if (checkEnemyClose && Functions::enemyClose(next)) break;

            if (MoveObstacle_tmp(target_pos, next)) {
                if (off == 0 && Distance(localPlayer->position, target_pos) >= MIN_MOVE_DISTANCE) {
                    localPlayer->ClickToMove(Move, localPlayer->Guid, target_pos);
                    return true;
                }

                if (Distance(localPlayer->position, next) >= MIN_MOVE_DISTANCE) {
                    localPlayer->ClickToMove(Move, localPlayer->Guid, next);
                    return true;
                }

                break;
            }

            last = next;
        }
    }

    return false;
}

bool MoveObstacleSwim_tmp(const Position& target_pos, const Position& start_pos) {
    constexpr int MAX_STEPS = 3;

    const float targetDist = Distance2D(start_pos, target_pos);
    if (targetDist < MIN_MOVE_DISTANCE) return true;

    const float base = std::atan2(target_pos.Y - start_pos.Y, target_pos.X - start_pos.X);
    const float cosDir = std::cos(base);
    const float sinDir = std::sin(base);
    Position last = start_pos;

    for (int i = 0; i < MAX_STEPS; i++) {
        const float remaining = Distance2D(last, target_pos);
        if (remaining < MIN_MOVE_DISTANCE) return true;

        const float step = remaining <= SWIM_STEP ? remaining : SWIM_STEP;
        Position next = BuildSwimStep(last, target_pos, cosDir, sinDir, step);

        if (Functions::Intersect(last, next, 1.25f)) return false;

        last = next;
        if (step < SWIM_STEP) return true;
    }

    return true;
}

bool Functions::MoveObstacleSwim(Position target_pos, bool checkEnemyClose) {
    return MoveObstacleSwimInternal(target_pos, checkEnemyClose);
}
