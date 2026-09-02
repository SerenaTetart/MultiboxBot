#pragma once

#include <unordered_map>
#include <cstdint>

enum class ResistanceFlags : uint32_t {
    None   = 0,
    Fire   = 1 << 0,
    Frost  = 1 << 1,
    Nature = 1 << 2,
    Shadow = 1 << 3,
    Arcane = 1 << 4
};

constexpr ResistanceFlags operator|(
    ResistanceFlags a,
    ResistanceFlags b)
{
    return static_cast<ResistanceFlags>(
        static_cast<uint32_t>(a) |
        static_cast<uint32_t>(b)
    );
}

constexpr bool HasResistance(
    ResistanceFlags flags,
    ResistanceFlags resistance)
{
    return (
        static_cast<uint32_t>(flags) &
        static_cast<uint32_t>(resistance)
    ) != 0;
}

inline const std::unordered_map<int, ResistanceFlags> npcResistances = {
    { 3651, ResistanceFlags::Frost },
    // { 12348, ResistanceFlags::Nature | ResistanceFlags::Shadow }
};

inline ResistanceFlags GetNPCResistances(int entryID) {
    auto it = npcResistances.find(entryID);

    if (it == npcResistances.end())
        return ResistanceFlags::None;

    return it->second;
}