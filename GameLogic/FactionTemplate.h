#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

struct FactionTemplateEntry
{
    uint32_t ID;              // col 0
    uint32_t Faction;         // col 1  -> index vers Faction.dbc
    uint32_t Flags;           // col 2
    uint32_t FactionGroup;    // col 3  (ourMask)
    uint32_t FriendGroup;     // col 4  (friendlyMask)
    uint32_t EnemyGroup;      // col 5  (hostileMask)
    uint32_t EnemyFaction[4]; // col 6..9   "always enemy"
    uint32_t FriendFaction[4];// col 10..13 "always friend"
};

struct DbcHeader
{
    char     magic[4];        // "WDBC"
    uint32_t recordCount;
    uint32_t fieldCount;
    uint32_t recordSize;      // bytes per record
    uint32_t stringBlockSize; // size of string table (not used here)
};

class FactionTemplateDBC {
public:
    explicit FactionTemplateDBC(const std::string& path) { load(path); }
    const FactionTemplateEntry* Get(uint32_t id) const;
    bool isNeutral(uint32_t ID);
private:
    std::unordered_map<uint32_t, FactionTemplateEntry> byId;
    const std::unordered_map<uint32_t, FactionTemplateEntry>& All() const { return byId; }
    static uint32_t readU32(const char* p);
    void load(const std::string& path);
    std::string GetModulePath();
};

extern FactionTemplateDBC FactionTemplate;