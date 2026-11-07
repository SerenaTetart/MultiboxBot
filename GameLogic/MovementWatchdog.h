#pragma once

#include <chrono>
#include <cmath>

class MovementWatchdog {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    static constexpr auto StallTimeout = std::chrono::milliseconds(2500);
    static constexpr float ProgressThreshold = 0.75f;
    static constexpr float ArrivalThreshold = 1.5f;

    template<class Point>
    void Record(const Point& from, const Point& to, TimePoint now) {
        if (!active_) {
            active_ = true;
            lastProgress_ = now;
            progressX_ = from.X;
            progressY_ = from.Y;
            progressZ_ = from.Z;
        } else {
            UpdateProgress(from, now);
        }
        // Replanning can change the waypoint every frame. It must not
        // hide a physically stationary character from the watchdog.
        // Changing waypoints does not reset the stall timeout without actual progress.
        goalX_ = to.X;
        goalY_ = to.Y;
        goalZ_ = to.Z;
    }

    template<class Point>
    bool Stalled(const Point& current, TimePoint now) {
        if (!active_) return false;
        if (Distance3D(current.X, current.Y, current.Z,
            goalX_, goalY_, goalZ_) <= ArrivalThreshold) {
            Clear();
            return false;
        }
        UpdateProgress(current, now);
        return now - lastProgress_ >= StallTimeout;
    }

    template<class Point>
    bool Towards(const Point& goal) const {
        return active_ && XYDistance(goal.X, goal.Y, goalX_, goalY_) <= ArrivalThreshold &&
            std::abs(goal.Z - goalZ_) <= ArrivalThreshold;
    }

    bool Active() const { return active_; }
    void Clear() { active_ = false; }

private:
    static float XYDistance(float ax, float ay, float bx, float by) {
        return std::hypot(ax - bx, ay - by);
    }
    static float Distance3D(float ax, float ay, float az, float bx, float by, float bz) {
        return std::hypot(XYDistance(ax, ay, bx, by), az - bz);
    }

    template<class Point>
    void UpdateProgress(const Point& current, TimePoint now) {
        if (Distance3D(current.X, current.Y, current.Z,
            progressX_, progressY_, progressZ_) >= ProgressThreshold) {
            progressX_ = current.X;
            progressY_ = current.Y;
            progressZ_ = current.Z;
            lastProgress_ = now;
        }
    }

    bool active_ = false;
    float goalX_ = 0.0f, goalY_ = 0.0f, goalZ_ = 0.0f;
    float progressX_ = 0.0f, progressY_ = 0.0f, progressZ_ = 0.0f;
    TimePoint lastProgress_{};
};
