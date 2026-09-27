#pragma once
#include <cstdint>
#include <cstddef>

struct SpellRecordPartial {
    std::byte unknown_000[0x1E0];

    const char* name[9]; // 0x1E0
    const char* rank[9]; // 0x204
};

struct SpellSlotData {
    int slot = -1;
    int32_t id = 0;

    const char* name = nullptr;
    int rank = -1;

    explicit operator bool() const {
        return id > 0 && name != nullptr;
    }
};