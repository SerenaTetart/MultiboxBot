#include "Game.h"
#include "Functions.h"
#include "FunctionsLua.h"
#include "MemoryManager.h"
#include <time.h>

bool Game::Loot() {
	// Loot Mineral/Herbs
	if (herbalismLevel > 0 || miningLevel > 0) {
		for (unsigned int i = 0; i < ListGameObjects.size(); i++) {
			if (ListGameObjects[i].gatherType == 0) continue;
			else if (IsInGroup && Leader != NULL && Leader->position.DistanceTo(ListGameObjects[i].position) > 40.0f)
				continue;
			else if (Functions::enemyClose(ListGameObjects[i].position)) continue;
			int skillLevel = herbalismLevel; if (ListGameObjects[i].gatherType == 1) skillLevel = miningLevel;
			if ((ListGameObjects[i].gatherType == 1 && skillLevel >= ListGameObjects[i].level && skillLevel < ListGameObjects[i].level + 100)
				|| (ListGameObjects[i].gatherType == 2 && skillLevel >= ListGameObjects[i].level && skillLevel < ListGameObjects[i].level + 100)) {
				if (ListGameObjects[i].position.DistanceTo(localPlayer->position) < 5.0f) {
					if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
						ThreadSynchronizer::pressKey(0x28);
						ThreadSynchronizer::releaseKey(0x28);
						Moving = 0;
					}
					ThreadSynchronizer::RunOnMainThread([i]() {
						if (localPlayer->isMounted) Dismount();
						Functions::InteractObject(ListGameObjects[i].Pointer, 1);
					});
					return true;
				}
				else if (!localPlayer->isMoving) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						Functions::MoveTo(ListGameObjects[i].position, 11);
					});
					if (Moving != 0) return true;
				}
				else if (Moving == 11) return true;
			}
		}
	}
	// Loot NPC
	std::vector<bool> already_looted;
	for (int y = 0; y <= NumGroupMembers; y++) {
		already_looted.push_back(false);
	}
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		bool lootable = (ListUnits[i].dynamic_flags & DYNAMICFLAG_CANBELOOTED);
		bool skinnable = (ListUnits[i].flags & UNIT_FLAG_SKINNABLE);
		float min_dist = localPlayer->position.DistanceTo(ListUnits[i].position);
		if (min_dist > 40.0f) continue;
		if (lootable) {
			bool skip = false;
			for (unsigned int y = 0; y < LootHistory.size(); y++) {
				if (get<0>(LootHistory[y]) == ListUnits[i].Guid) {
					skip = true;
					break;
				}
			}
			if (skip) continue;
			int player_close = 0;
			for (int y = 1; y <= NumGroupMembers; y++) {
				if (GroupMember[y] == NULL || (already_looted[y] == true) || (Leader != NULL && Leader->Guid == GroupMember[y]->Guid && !MCAutoMove && Leader->indexGroup == 0)) continue;
				float dist = GroupMember[y]->position.DistanceTo(ListUnits[i].position);
				if (dist < min_dist) {
					min_dist = dist;
					player_close = y;
				}
			}
			already_looted[player_close] = true;
			if (player_close != 0) continue;
			else if (ListUnits[i].position.DistanceTo(localPlayer->position) < 4.0f) {
				if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
					ThreadSynchronizer::pressKey(0x28);
					ThreadSynchronizer::releaseKey(0x28);
					Moving = 0;
				}
				else if (localPlayer->speed == 0.0f) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						if (localPlayer->isMounted) Dismount();
						Functions::LootUnit(ListUnits[i].Pointer, 1);
					});
					LootHistory.push_back(std::tuple<unsigned long long, time_t>(ListUnits[i].Guid, time(0)));
				}
				return true;
			}
			else if (!Functions::enemyClose(ListUnits[i].position)) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::MoveTo(ListUnits[i].position, 11);
				});
				if (Moving != 0) return true;
			}
			else if (Moving == 11) return true;
		}
		else if (skinnable && skinningLevel > 0 && (ListUnits[i].level <= 20 && ((ListUnits[i].level - 10) * 10 <= skinningLevel && !Functions::enemyClose(ListUnits[i].position))
			|| (ListUnits[i].level > 20 && (ListUnits[i].level * 5 <= skinningLevel)))) {
			if (ListUnits[i].position.DistanceTo(localPlayer->position) < 4.0f) {
				if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
					ThreadSynchronizer::pressKey(0x28);
					ThreadSynchronizer::releaseKey(0x28);
					Moving = 0;
				}
				else if (localPlayer->speed == 0.0f) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						if (localPlayer->isMounted) Dismount();
						Functions::LootUnit(ListUnits[i].Pointer, 1);
						});
					LootHistory.push_back(std::tuple<unsigned long long, time_t>(ListUnits[i].Guid, time(0)));
				}
				return true;
			}
			else if (!Functions::enemyClose(ListUnits[i].position)) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::MoveTo(ListUnits[i].position, 11);
				});
				if (Moving != 0) return true;
			}
			else if (Moving == 11) return true;
		}
	}
	return false;
}

bool Game::Trade() {
	// Trade
	bool traded = false;
	for (int y = 1; y <= NumGroupMembers; y++) {
		for (unsigned int i = 0; i < leaderInfos.size(); i++) {
			if (GroupMember[y] != NULL &&
				GroupMember[y]->position.DistanceTo(localPlayer->position) < 8.0f &&
				GroupMember[y]->name == get<0>(leaderInfos[i])
			) {
				if ((get<2>(leaderInfos[i]) == 4 || get<3>(leaderInfos[i]) == 4)) {
					// Tailoring
					int listID[] = { 2589, 2592, 3182, 4306, 4337, 4338, 10285, 14047, 14227, 14256 };
					for (const auto& item : virtualInventory) {
						for (unsigned int z = 0; z < 10; z++) {
							if (get<2>(item) == listID[z]) {
								ThreadSynchronizer::RunOnMainThread([item, y]() {
									FunctionsLua::PickupItem(get<0>(item), get<1>(item));
									FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
								});
								traded = true;
							}
						}
					}
				}
				if ((get<2>(leaderInfos[i]) == 5 || get<3>(leaderInfos[i]) == 5)) {
					// Leatherworking
					int listID[] = { 783, 2318, 2319, 2934, 4232, 4234, 4235, 4304, 7392, 7428, 8154, 8165, 8167, 8368, 8169 };
					for (const auto& item : virtualInventory) {
						for (unsigned int z = 0; z < 15; z++) {
							if (get<2>(item) == listID[z]) {
								ThreadSynchronizer::RunOnMainThread([item, y]() {
									FunctionsLua::PickupItem(get<0>(item), get<1>(item));
									FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
								});
								traded = true;
							}
						}
					}
				}
				if ((get<2>(leaderInfos[i]) == 6 || get<3>(leaderInfos[i]) == 6)) {
					// Blacksmithing
					int listID[] = { 2770, 2771, 2772, 2775, 2776, 2835, 2836, 2838, 3858, 7912, 10620, 11370 };
					for (const auto& item : virtualInventory) {
						for (unsigned int z = 0; z < 12; z++) {
							if (get<2>(item) == listID[z]) {
								ThreadSynchronizer::RunOnMainThread([item, y]() {
									FunctionsLua::PickupItem(get<0>(item), get<1>(item));
									FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
								});
								traded = true;
							}
						}
					}
				}
				if ((get<2>(leaderInfos[i]) == 8 || get<3>(leaderInfos[i]) == 8)) {
					// Alchemy
					int listID[] = { 765, 785, 2447, 2449, 2450, 2452, 2453, 3355, 3356, 3357, 3358, 3369, 3818, 3819, 3820, 3821, 4625, 8831, 8836, 8838, 8839, 8845 };
					for (const auto& item : virtualInventory) {
						for (unsigned int z = 0; z < 22; z++) {
							if (get<2>(item) == listID[z]) {
								ThreadSynchronizer::RunOnMainThread([item, y]() {
									FunctionsLua::PickupItem(get<0>(item), get<1>(item));
									FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
								});
								traded = true;
							}
						}
					}
				}
			}
			if (traded) return true;
		}
	}
	return false;
}