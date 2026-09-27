#pragma once

#include <string>

struct InventoryItem
{
    int bag;
    int slot;
    int id;

    std::string texture;

    int count;
    bool locked;
    int quality;
    bool readable;
    bool lootable;

    int minLevel;
    std::string type;
    std::string subtype;
};