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

uintptr_t WoWObject::GetDescriptorPtr() {
    return *(uintptr_t*)(Pointer + 0x8);
}

/* === Unit === */

WoWUnit::WoWUnit(uintptr_t pointer, unsigned long long guid, ObjectType objType) : WoWObject(pointer, guid, objType) {
    uintptr_t descriptor = WoWObject::GetDescriptorPtr();

    float x = *(float*)(Pointer + POS_X_OFFSET);
    float y = *(float*)(Pointer + POS_Y_OFFSET);
    float z = *(float*)(Pointer + POS_Z_OFFSET);
    position = Position(x, y, z);

    int health = *(int*)(descriptor + HEALTH_OFFSET);
    int max_health = *(int*)(descriptor + MAX_HEALTH_OFFSET);
    prctHP = ((float)health / (float)max_health) * 100;
    hpLost = max_health - health;
    int max_mana = *(int*)(descriptor + MAXMANA_OFFSET);
    if (max_mana > 0) {
        int mana = *(int*)(descriptor + MANA_OFFSET);
        prctMana = ((float)mana / (float)max_mana) * 100;
    }
    else prctMana = -1;
    rage = (*(int*)(descriptor + RAGE_OFFSET)/10);
    energy = *(int*)(descriptor + ENERGY_OFFSET);

    flags = *(UnitFlags*)(descriptor + UNIT_FLAG_OFFSET);
    movement_flags = *(MovementFlags*)(Pointer + MOVEMENT_FLAG_OFFSET);
    dynamic_flags = *(DynamicFlags*)(descriptor + DYNAMIC_FLAG_OFFSET);

    isdead = false; if (health <= 1 && !(flags & UNIT_FLAG_FEIGN_DEATH)) isdead = true;

    uintptr_t currentBuffOffset = BUFF_BASE_OFFSET;
    for (int i = 0; i < 30; i++) {
        buff[i] = *(int*)(descriptor + currentBuffOffset);
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

    speed = *(float*)(Pointer + SPEED_OFFSET);
    isMoving = ((speed > 0) || (movement_flags & MOVEFLAG_FORWARD) || (movement_flags & MOVEFLAG_BACKWARD));
    targetGuid = *(unsigned long long*)(descriptor + TARGET_GUID_OFFSET);
    facing = *(float*)(Pointer + FACING_OFFSET);
    level = *(int*)(descriptor + LEVEL_OFFSET);
    if (objType == Unit) name = (char*)(*(uintptr_t*)(*(uintptr_t*)(Pointer + NAME_OFFSET)));
    else {
        uintptr_t namePtr = NAME_BASE_OFFSET;
        while (true) {
            unsigned long long nextGuid = *(unsigned long long*)(namePtr + NEXT_NAME_OFFSET);

            if (nextGuid != guid) namePtr = *(int*)(namePtr);
            else break;
        }
        name = (char*)(namePtr + PLAYER_NAME_OFFSET);
    }

    channelInfo = *(int*)(descriptor + CHANNEL_OFFSET);

    factionTemplateID = *(int*)(descriptor + FACTION_TEMPLATE_ID_OFFSET);

    unitReaction = Neutral;
    attackable = false;
    
    indexGroup = -1;
    role = -1;
}

int WoWUnit::getHealth() {
    return *(int*)(WoWObject::GetDescriptorPtr() + HEALTH_OFFSET);
}

int WoWUnit::getMaxHealth() {
    return *(int*)(WoWObject::GetDescriptorPtr() + MAX_HEALTH_OFFSET);
}

bool WoWUnit::hasBuff(int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        for (int y = 0; y < 30; y++) {
            if (IDs[i] == buff[y]) return true;
        }
    }
    return false;
}

bool WoWUnit::hasDebuff(int* IDs, int size) {
    for (int i = 0; i < size; i++) {
        for (int y = 0; y < 16; y++) {
            if (IDs[i] == debuff[y]) return true;
        }
    }
    return false;
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

/* === Local Player === */

LocalPlayer::LocalPlayer(uintptr_t pointer, unsigned long long guid, ObjectType objType)
    : WoWUnit(pointer, guid, objType) {
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
