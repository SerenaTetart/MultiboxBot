#pragma once

#include <algorithm>
#include <cmath>

namespace DangerZoneGeometry {
    constexpr float SafetyMargin = 1.5f;
    constexpr float MaxVerticalSeparation = 6.0f;

    template <typename Point>
    bool Contains(const Point& point, const Point& center, float radius) {
        if (radius <= 0.0f || std::abs(point.Z - center.Z) > MaxVerticalSeparation) return false;
        const float dx = point.X - center.X;
        const float dy = point.Y - center.Y;
        return dx * dx + dy * dy < radius * radius;
    }

    // Reject any straight movement cutting a dangerous disc. If already inside,
    // a strictly outward step is permitted so the character can escape.
    template <typename Point>
    bool BlocksSegment(const Point& from, const Point& to, const Point& center, float radius) {
        if (radius <= 0.0f) return false;
        const float fromDz = from.Z - center.Z, toDz = to.Z - center.Z;
        // Ignore another floor only when both endpoints are on the same side
        // of it. A vertical crossing can still intersect the danger layer.
        if ((fromDz > MaxVerticalSeparation && toDz > MaxVerticalSeparation) ||
            (fromDz < -MaxVerticalSeparation && toDz < -MaxVerticalSeparation))
            return false;

        const float sx = from.X - center.X, sy = from.Y - center.Y;
        const float dx = to.X - from.X, dy = to.Y - from.Y;
        const float length2 = dx * dx + dy * dy;
        const float start2 = sx * sx + sy * sy;
        const float radius2 = radius * radius;
        if (length2 < 0.000001f) return start2 < radius2;

        const float end2 = (sx + dx) * (sx + dx) + (sy + dy) * (sy + dy);
        if (start2 < radius2) {
            return !(sx * dx + sy * dy >= 0.0f && end2 > start2 + 0.0001f);
        }

        const float t = std::clamp(-(sx * dx + sy * dy) / length2, 0.0f, 1.0f);
        const float closestX = sx + t * dx, closestY = sy + t * dy;
        return closestX * closestX + closestY * closestY < radius2;
    }
}
