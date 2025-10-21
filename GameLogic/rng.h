#pragma once
#include <random>
#include <array>

namespace RNG {
    inline std::mt19937& engine() {
        static std::mt19937 eng = [] {
            std::random_device rd;
            // On prélève plusieurs entiers pour un meilleur ensemencement
            std::seed_seq seq{
                rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()
            };
            return std::mt19937(seq);
            }();
        return eng;
    }
}