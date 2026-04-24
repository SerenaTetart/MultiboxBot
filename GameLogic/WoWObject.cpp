#include "WoWObject.h"

#include <string>

#include "MemoryManager.h"
#include <iostream>

std::vector<WoWUnit> ListUnits;
std::vector<WoWGameObject> ListGameObjects;
LocalPlayer* localPlayer = NULL;

Position::Position() {
    X = 0;
    Y = 0;
    Z = 0;
}

Position::Position(float x, float y, float z) {
    X = x;
    Y = y;
    Z = z;
}

bool Position::operator==(const Position& other) const {
    return X == other.X && Y == other.Y && Z == other.Z;
}

std::string Position::ToString() {
    std::string txt = "x: " + std::to_string(X) + " y: " + std::to_string(Y) + " z: " + std::to_string(Z);
    return txt;
}

float Position::DistanceTo(Position position) {
    float deltaX = X - position.X;
    float deltaY = Y - position.Y;
    float deltaZ = Z - position.Z;

    return std::sqrt((deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ));
}

float Position::DistanceTo2D(Position position) {
    float deltaX = X - position.X;
    float deltaY = Y - position.Y;

    return (float)sqrt((deltaX * deltaX) + (deltaY * deltaY));
}

/* === Object === */

WoWObject::WoWObject(uintptr_t pointer, unsigned long long guid, ObjectType objType) {
    Guid = guid;
    Pointer = pointer;
    objectType = objType;
}

uintptr_t WoWObject::GetDescriptorPtr(uintptr_t pointer) {
    if (pointer == NULL) return NULL;
    else return *(uintptr_t*)(pointer + 0x8);
}

/* === Unit === */

WoWUnit::WoWUnit(uintptr_t pointer, unsigned long long guid, ObjectType objType) : WoWObject(pointer, guid, objType) {
    uintptr_t descriptor = WoWObject::GetDescriptorPtr(pointer);
    movement_flags = *(MovementFlags*)(Pointer + MOVEMENT_FLAG_OFFSET);
    speed = *(float*)(Pointer + SPEED_OFFSET);
    isMoving = ((speed > 0) || (movement_flags & MOVEFLAG_FORWARD) || (movement_flags & MOVEFLAG_BACKWARD));
    if (descriptor != NULL) {
        health = *(int*)(descriptor + HEALTH_OFFSET);
        int max_health = *(int*)(descriptor + MAX_HEALTH_OFFSET);
        prctHP = ((float)health / (float)max_health) * 100;
        hpLost = max_health - health;
        int max_mana = *(int*)(descriptor + MAXMANA_OFFSET);
        if (max_mana > 0) {
            int mana = *(int*)(descriptor + MANA_OFFSET);
            prctMana = ((float)mana / (float)max_mana) * 100;
        }
        else prctMana = -1;
        rage = (*(int*)(descriptor + RAGE_OFFSET) / 10);
        energy = *(int*)(descriptor + ENERGY_OFFSET);

        flags = *(UnitFlags*)(descriptor + UNIT_FLAG_OFFSET);
        dynamic_flags = *(DynamicFlags*)(descriptor + DYNAMIC_FLAG_OFFSET);
        createdBy = *(int*)(descriptor + CREATED_BY_SPELL_OFFSET);

        isdead = false; if (health <= 1 && !(flags & UNIT_FLAG_FEIGN_DEATH)) isdead = true;

        bool travelForm = false;
        uintptr_t currentBuffOffset = BUFF_BASE_OFFSET;
        for (int i = 0; i < 40; i++) {
            buff[i] = *(int*)(descriptor + currentBuffOffset);
            if (buff[i] == 783) travelForm = true;
            currentBuffOffset += 4;
        }
        uintptr_t currentDebuffOffset = DEBUFF_BASE_OFFSET;
        for (int i = 0; i < 16; i++) {
            debuff[i] = *(int*)(descriptor + currentDebuffOffset);
            currentDebuffOffset += 4;
        }

        typedef CreatureType __fastcall func(uintptr_t);
        func* getCreatureType = (func*)GET_CREATURE_TYPE_FUN_PTR;
        creatureType = getCreatureType(Pointer);

        targetGuid = *(unsigned long long*)(descriptor + TARGET_GUID_OFFSET);
        facing = *(float*)(Pointer + FACING_OFFSET);
        level = *(int*)(descriptor + LEVEL_OFFSET);
        if (objType == Unit) {
            uintptr_t cachePtr = *(uintptr_t*)(Pointer + CREATURE_CACHE_OFFSET);
            name = (char*)(*(uintptr_t*)(cachePtr));
            rank = *(int*)(cachePtr + 0x20);
        }
        else {
            uintptr_t namePtr = NAME_BASE_OFFSET;
            while (true) {
                unsigned long long nextGuid = *(unsigned long long*)(namePtr + NEXT_NAME_OFFSET);

                if (nextGuid != guid) namePtr = *(int*)(namePtr);
                else break;
            }
            name = (char*)(namePtr + PLAYER_NAME_OFFSET);
            rank = 0;
        }

        combatReach = *(float*)(descriptor + COMBAT_REACH_OFFSET);
        channelInfo = *(int*)(descriptor + CHANNEL_OFFSET);

        factionTemplateID = *(int*)(descriptor + FACTION_TEMPLATE_ID_OFFSET);

        mountModelID = *(int*)(descriptor + MOUNT_MODEL_ID_OFFSET);
        isMounted = (travelForm || ((mountModelID != 0) && !(flags & UNIT_FLAG_TAXI_FLIGHT)));
    }

    float x = *(float*)(Pointer + POS_X_OFFSET);
    float y = *(float*)(Pointer + POS_Y_OFFSET);
    float z = *(float*)(Pointer + POS_Z_OFFSET);
    position = Position(x, y, z);

    entryID = ((uint32_t)((guid >> 24) & 0xFFFF));
}

int WoWUnit::hasBuff(const int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        for (int y = 0; y < 40; y++) {
            if (IDs[i] == buff[y]) return IDs[i];
        }
    }
    return 0;
}

bool WoWUnit::hasBuff(int buffID) {
    for (int y = 0; y < 40; y++) {
        if (buffID == buff[y]) return true;
    }
    return false;
}

bool WoWUnit::hasDebuff(int debuffID) {
    for (int y = 0; y < 16; y++) {
        if (debuffID == debuff[y]) return true;
    }
    return false;
}

int WoWUnit::hasDebuff(const int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        for (int y = 0; y < 16; y++) {
            if (IDs[i] == debuff[y]) return IDs[i];
        }
    }
    return 0;
}

int WoWUnit::getNbrDebuff() {
    int total = 0;
    for (int i = 0; i < 16; i++) {
        if (debuff[i] != 0) total++;
    }
    return total;
}

bool WoWUnit::isFacing(Position pos, float angle) {
    float f = atan2f(pos.Y - position.Y, pos.X - position.X);
    float PI = 2 * acosf(0.0);
    if (f < 0.0f) f += PI * 2.0f;
    else if (f > PI * 2) f -= PI * 2.0f;
    if (abs(f - facing) < angle) return true;
    else return false;
}

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool WoWUnit::isBehind(WoWUnit* target) {
    float dx = this->position.X - target->position.X;
    float dy = (this->position.Y - target->position.Y);

    float len2 = dx * dx + dy * dy;

    float invLen = 1.0f / std::sqrt(len2);
    float rx = dx * invLen;
    float ry = dy * invLen;

    float facing = target->facing;

    float fx = std::cos(facing);
    float fy = std::sin(facing);

    float dot = fx * rx + fy * ry;
    dot = clampf(dot, -1.0f, 1.0f); // numeric safety

    const float coneHalfAngle = 0.4f; // ≈ 23°
    float threshold = -std::cos(coneHalfAngle);

    bool behind = (dot <= threshold);

    return behind;
}

bool WoWUnit::isChanneling(int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        if (IDs[i] == channelInfo) return true;
    }
    return false;
}

UnitReaction WoWUnit::getUnitReaction(uintptr_t unitPtr2) {
    typedef UnitReaction(__thiscall* func)(uintptr_t unitPtr1, uintptr_t unitPtr2);
    func function = (func)GET_UNIT_REACTION_FUN_PTR;
    return function(Pointer, unitPtr2);
}

bool WoWUnit::canAttack(uintptr_t unitPtr2) {
    typedef bool(__thiscall* func)(uintptr_t unitPtr1, uintptr_t unitPtr2);
    func function = (func)CAN_ATTACK_UNIT_FUN_PTR;
    return function(Pointer, unitPtr2);
}

bool WoWUnit::isCrowdControlled() {
    // Fear, stun, confusion...
    if ((flags & UNIT_FLAG_FLEEING) || (flags & UNIT_FLAG_CONFUSED) || (flags & UNIT_FLAG_STUNNED)) return true;
    else return false;
}

bool WoWUnit::isElite() {
    if (rank >= 1 && rank <= 3) return true;
    else return false;
}

/* === Local Player === */

int GetSpellModifierFromSpellId(int spellId) {
    static const std::unordered_map<int, int> ModifierBySpell = {
        // Healing + Damage mod
        {9342, 13}, {9343, 14}, {9344, 15},
        {9395, 5},  {9396, 6},  {9397, 7},
        {9415, 9},  {9416, 11}, {9417, 12},
        {9346, 18}, {14047, 23}, {14054, 27},
        {14248, 21}, {14254, 19}, {14798, 30},
        {14799, 20}, {15714, 22}, {17367, 32},
        {18049, 26}, {18052, 34}, {18054, 37},
        {18056, 40}, {26395, 72}, {28141, 150},
        {28693, 95}, {26142, 53}, {28360, 49},
        {28841, 113}, {17280, 43}, {23728, 84},
        {30777, 23}, {9398, 8}, {14127, 28},
        {17493, 44}, {18057, 41}, {24196, 47},
        {26460, 76}, {28687, 85}, {28767, 51},
        {9345, 16}, {9393, 2}, {9394, 4}, {13881, 29},
        {15715, 25}, {18050, 33}, {23730, 64}, {25111, 24},
        // Healing mod
        {9318, 33}, {17371, 44}, {18036, 55}, {7681, 15},
        {7676, 4}, {18039, 62}, {7678, 9}, {18032, 42},
        {18035, 51}, {9314, 24}, {9408, 22}, {18045, 75},
        {23593, 92}, {22748, 55}, {29369, 134}, {15696, 53},
        {18048, 81}, {9315, 26}, {18030, 37}, {18043, 70},
        {18046, 77}, {26814, 187}, {28151, 280}, {7679, 11},
        {9316, 29}, {18029, 35}, {18037, 57}, {18041, 66},
        {23264, 106}, {25067, 30}, {26154, 90}, {26461, 143},
        {28805, 238}, {7675, 2}, {9406, 18}, {17320, 84},
        {18038, 59}, {18042, 68}, {18044, 73}, {7680, 13},
        {9317, 31}, {18033, 46}, {18034, 48}, {18047, 79}, {23796, 24}
    };

    auto it = ModifierBySpell.find(spellId);
    return it != ModifierBySpell.end() ? it->second : 0;
}

LocalPlayer::LocalPlayer(uintptr_t pointer, unsigned long long guid, ObjectType objType)
    : WoWUnit(pointer, guid, objType) {
    uintptr_t descriptor = GetDescriptorPtr(pointer);
    if (descriptor != NULL) {
        typedef uintptr_t(__thiscall* func)(uintptr_t ptr, int itemID, uintptr_t unknown, int unused1, int unused2, char unused3);
        func GetItemCacheEntry = (func)0x0055BA30;

        uintptr_t listOffsetEquipment[] = { 0x410, 0x440, 0x470, 0x4d0, 0x500, 0x530, 0x560, 0x590, 0x5c0, 0x620, 0x650, 0x680, 0x6b0, 0x6e0, 0x710 };
        bonusHealing = 0;
        for (int i = 0; i < 15; i++) {
            int itemID = *(int*)(descriptor + listOffsetEquipment[i]);
            uintptr_t cacheEntryPTR = GetItemCacheEntry((uintptr_t)0x00C0E2A0, itemID, (uintptr_t)0x0, 0, 0, (char)0);

            if (cacheEntryPTR != NULL) {
                //name = (char*)(*(uintptr_t*)(cacheEntryPTR + 0x8));

                int equipSlot = *(int*)(cacheEntryPTR + 0x2C);
                if (equipSlot > 0) {
                    int intellect = *(int*)(cacheEntryPTR + 0x90);
                    int spirit = *(int*)(cacheEntryPTR + 0x94);
                    int stamina = *(int*)(cacheEntryPTR + 0x98);
                    int strength = *(int*)(cacheEntryPTR + 0x9c);
                    int armor = *(int*)(cacheEntryPTR + 0xf4);
                    int frostRes = *(int*)(cacheEntryPTR + 0x104);
                    int shadowRes = *(int*)(cacheEntryPTR + 0x108);
                    int bonusSpell[3] = { *(int*)(cacheEntryPTR + 0x11c), *(int*)(cacheEntryPTR + 0x120), *(int*)(cacheEntryPTR + 0x124) };

                    for (int i = 0; i < 3; i++) {
                        bonusHealing += GetSpellModifierFromSpellId(bonusSpell[i]);
                    }
                }
            }
        }
    }

    className = "";
    castInfo = *(int*)(CASTING_STATIC_OFFSET);
    targetGuid = *(unsigned long long*)LOCKED_TARGET_STATIC_OFFSET;
    float x = *(float*)(PLAYER_CORPSE_POSITION_X);
    float y = *(float*)(PLAYER_CORPSE_POSITION_Y);
    float z = *(float*)(PLAYER_CORPSE_POSITION_Z);
    corpse_position = Position(x, y, z);
    zoneID = *(int*)(ZONE_ID_OFFSET);
    //std::cout << "corpse pos: " << corpse_position.X << ", " << corpse_position.Y << ", " << corpse_position.Z << "\n";
}

void LocalPlayer::ClickToMove(ClickType clickType, unsigned long long interactGuid, Position pos) {
    float xyz[3] = { pos.X, pos.Y, pos.Z };
    unsigned long long* interactGuidPtr = &interactGuid;
    typedef void (__thiscall* func)(uintptr_t, ClickType, unsigned long long*, float*, float);
    func function = (func)CLICK_TO_MOVE_FUN_PTR;
    function(Pointer, clickType, interactGuidPtr, xyz, 0.5f);
    if (clickType == Move) isMoving = true;
}

void LocalPlayer::SetTarget(unsigned long long tguid) {
    typedef void __stdcall func(unsigned long long tguid);
    func* function = (func*)SET_TARGET_FUN_PTR;
    function(tguid);
}

WoWUnit* LocalPlayer::getTarget() {
    for (unsigned int i = 0; i < ListUnits.size(); i++) {
        if (ListUnits[i].Guid == targetGuid) return (&ListUnits[i]);
    }
    return NULL;
}

Position LocalPlayer::getOppositeDirection(Position target_pos, float radius) {
    float m = (float)(target_pos.Y - position.Y) / (float)(target_pos.X - position.X);
    float p = position.Y - (m * position.X);
    float a = 1 + (m * m);
    float b = (-2 * position.X) - (2 * m * m * position.X);
    float c = (position.X * position.X) + (m * m * position.X * position.X) - (radius * radius);
    float delta = (b * b) - (4 * a * c);
    float x1 = (-b - sqrt(delta)) / (2 * a);
    float y1 = (m * x1 + p);
    float x2 = (-b + sqrt(delta)) / (2 * a);
    float y2 = (m * x2 + p);
    Position x1_pos = Position(x1, y1, position.Z);
    Position x2_pos = Position(x2, y2, position.Z);
    if (target_pos.DistanceTo(x1_pos) > target_pos.DistanceTo(x2_pos)) return x1_pos;
    else return x2_pos;
}

bool LocalPlayer::isCasting() {
    int casting = *(int*)(CASTING_STATIC_OFFSET);
    if (casting > 0) return true;
    else return false;
}

bool LocalPlayer::isCasting(int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        if (IDs[i] == castInfo) return true;
    }
    return false;
}

/* === Gameobject === */

WoWGameObject::WoWGameObject(uintptr_t pointer, unsigned long long guid, ObjectType objType) : WoWObject(pointer, guid, objType) {
    uintptr_t descriptor = 0x2a0;
    float x = *(float*)(Pointer + descriptor + 0x24);
    float y = *(float*)(Pointer + descriptor + 0x28);
    float z = *(float*)(Pointer + descriptor + 0x2C);
    position = Position(x, y, z);
    facing = *(float*)(Pointer + 0x30);
    displayID = *(int*)((char*)Pointer  + descriptor + 0x8);
    level = 0; gatherType = 0;
        // === Mining === //
    if (displayID == 310) gatherType = 1;
    else if (displayID == 315) { level = 65; gatherType = 1; }
    else if (displayID == 314) { level = 75; gatherType = 1; }
    else if (displayID == 312) { level = 125; gatherType = 1; }
    else if (displayID == 311) { level = 155; gatherType = 1; }
    else if (displayID == 313) { level = 175; gatherType = 1; }
    else if (displayID == 2571) { level = 230; gatherType = 1; }
    else if (displayID == 3951) { level = 245; gatherType = 1; }
    else if (displayID == 3952) { level = 275; gatherType = 1; }
    else if (displayID == 6650) { level = 305; gatherType = 1; }
        // === Herbalism === //
    else if (displayID == 269) { level = 0; gatherType = 2; }
    else if (displayID == 270) { level = 0; gatherType = 2; }
    else if (displayID == 414) { level = 15; gatherType = 2; }
    else if (displayID == 268) { level = 50; gatherType = 2; }
    else if (displayID == 271) { level = 70; gatherType = 2; }
    else if (displayID == 700) { level = 85; gatherType = 2; }
    else if (displayID == 358) { level = 100; gatherType = 2; }
    else if (displayID == 371) { level = 115; gatherType = 2; }
    else if (displayID == 357) { level = 120; gatherType = 2; }
    else if (displayID == 320) { level = 125; gatherType = 2; }
    else if (displayID == 677) { level = 150; gatherType = 2; }
    else if (displayID == 697) { level = 160; gatherType = 2; }
    else if (displayID == 698) { level = 170; gatherType = 2; }
    else if (displayID == 701) { level = 185; gatherType = 2; }
    else if (displayID == 701) { level = 185; gatherType = 2; }
    else if (displayID == 699) { level = 195; gatherType = 2; }
    else if (displayID == 2312) { level = 205; gatherType = 2; }
    else if (displayID == 2314) { level = 210; gatherType = 2; }
    else if (displayID == 2310) { level = 220; gatherType = 2; }
    else if (displayID == 2315) { level = 230; gatherType = 2; }
    else if (displayID == 2311) { level = 235; gatherType = 2; }
    else if (displayID == 389) { level = 245; gatherType = 2; }
    else if (displayID == 2313) { level = 250; gatherType = 2; }
    else if (displayID == 4652) { level = 260; gatherType = 2; }
    else if (displayID == 4635) { level = 270; gatherType = 2; }
    else if (displayID == 4633) { level = 280; gatherType = 2; }
    else if (displayID == 4632) { level = 285; gatherType = 2; }
    else if (displayID == 4634) { level = 290; gatherType = 2; }
    else if (displayID == 4636) { level = 300; gatherType = 2; }
}

/* === Items === */

WoWItem::WoWItem(uintptr_t pointer, unsigned long long guid, ObjectType objType) : WoWObject(pointer, guid, objType) {
    //itemID = *(int*)(pointer + 0x354);
}