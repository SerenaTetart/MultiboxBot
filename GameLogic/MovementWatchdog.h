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
        } else {
            UpdateProgress(from, now);
        }
        // Changing waypoints does not reset the stall timeout without actual progress.
        if (XYDistance(to.X, to.Y, goalX_, goalY_) > ArrivalThreshold)
            bestRemaining_ = XYDistance(from.X, from.Y, to.X, to.Y);
        goalX_ = to.X;
        goalY_ = to.Y;
    }

    template<class Point>
    bool Stalled(const Point& current, TimePoint now) {
        if (!active_) return false;
        if (XYDistance(current.X, current.Y, goalX_, goalY_) <= ArrivalThreshold) {
            Clear();
            return false;
        }
        UpdateProgress(current, now);
        return now - lastProgress_ >= StallTimeout;
    }

    template<class Point>
    bool Towards(const Point& goal) const {
        return active_ && XYDistance(goal.X, goal.Y, goalX_, goalY_) <= 1.5f;
    }

    bool Active() const { return active_; }

    void Clear() { active_ = false; }

private:
    static float XYDistance(float ax, float ay, float bx, float by) {
        return std::hypot(ax - bx, ay - by);
    }

    template<class Point>
    void UpdateProgress(const Point& current, TimePoint now) {
        const float remaining = XYDistance(current.X, current.Y, goalX_, goalY_);
        if (bestRemaining_ - remaining >= ProgressThreshold) {
            bestRemaining_ = remaining;
            lastProgress_ = now;
        }
    }

    bool active_ = false;
    float goalX_ = 0.0f, goalY_ = 0.0f;
    float bestRemaining_ = 0.0f;
    TimePoint lastProgress_{};
};
