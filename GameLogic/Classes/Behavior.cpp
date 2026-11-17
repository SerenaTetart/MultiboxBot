#include "../ListAI.h"
#include "../FactionTemplate.h"
#include <iostream>

static int getCreaturePriority(WoWUnit* unit) {
	if (Functions::enemyClose(unit->position)) return 999; // Avoid enemies not aggro
	else if (unit->creatureType == Totem) return 0; //Guardian
	else if (unit->rank == 0 || unit->rank == 4) return 1; //Normal
	else if (unit->rank == 1 || unit->rank == 2) return 2; //Elite
	else if (unit->rank == 3) return 3; //Boss
	else return -1;
}

static bool isPrioritary(WoWUnit* unit1, WoWUnit* unit2) {
	int priority1 = getCreaturePriority(unit1);
	int priority2 = getCreaturePriority(unit2);
	if (priority1 < priority2) return true;
	else if (priority1 == priority2 && unit1->level < unit2->level) return true;
	else if (priority1 == priority2 && unit1->level == unit2->level && unit1->health < unit2->health) return true;
	else return false;
}

bool ListAI::DPSTargeting() {
	if ((Leader != NULL) && (Leader->Guid == localPlayer->Guid) && !MCAutoMove && Leader->indexGroup == 0) return false;
	else if (nbrEnemyPlayer > 0) {
		if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable) {
			WoWUnit* target = NULL; float minDist = INFINITY;
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (
					(ListUnits[i].isInCombatOrEncounter() || (ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED))
					&& ListUnits[i].attackable
					&& !(ListUnits[i].flags & UNIT_FLAG_POSSESSED)
					&& !ListUnits[i].isdead
					&& !ListUnits[i].isFromGroup
					&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID))
					&& ListUnits[i].position.DistanceTo(localPlayer->position) < minDist
					) {
					target = &ListUnits[i];
					minDist = ListUnits[i].position.DistanceTo(localPlayer->position);
				}
			}
			if (target != NULL) {
				localPlayer->SetTarget(target->Guid);
				return true;
			}
		}
	}
	else {
		WoWUnit* target = NULL;
		for (unsigned int i = 0; i < ListUnits.size(); i++) {
			if (
				(ListUnits[i].isInCombatOrEncounter() || ListUnits[i].creatureType == Totem)
				&& ListUnits[i].attackable
				&& (inInstance || (ListUnits[i].dynamic_flags & DYNAMICFLAG_TAPPEDBYME))
				&& !(ListUnits[i].flags & UNIT_FLAG_POSSESSED)
				&& !ListUnits[i].isdead
				&& !ListUnits[i].isFromGroup
				&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID))
				&& (target == NULL || isPrioritary(&ListUnits[i], target))
				) {
				target = &ListUnits[i];
			}
		}
		if (target != NULL && target->Guid != localPlayer->targetGuid) {
			localPlayer->SetTarget(target->Guid);
			return true;
		}
		else if (targetUnit == NULL || !targetUnit->attackable) {
			for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
				if (HasAggro[i].size() > 0) {
					localPlayer->SetTarget(HasAggro[i][0]->Guid);
					return true;
				}
			}
		}
	}
	return false;
}

bool ListAI::TankTargeting() {
	if ((Leader == NULL) || (Leader->Guid != localPlayer->Guid) || MCAutoMove || Leader->indexGroup != 0) {
		WoWUnit* target = NULL; int maxPriority = -1; float minDist = INFINITY;
		for (int i = NumGroupMembers; i >= 0; i--) {
			if (GroupMember[i] == NULL || GroupMember[i]->role == 0) continue;
			for (unsigned int j = 0; j < HasAggro[i].size(); j++) {
				WoWUnit* unit = HasAggro[i][j];
				if (unit == NULL || unit->isdead || !unit->attackable || unit->isFromGroup || (unit->flags & UNIT_FLAG_POSSESSED)) continue;
				bool targetFocusingTank = (unit->targetGuid == localPlayer->Guid);
				for (int k = 0; k <= NumGroupMembers && !targetFocusingTank; k++) {
					if (GroupMember[k] != NULL && GroupMember[k]->role == 0 && unit->targetGuid == GroupMember[k]->Guid) {
						targetFocusingTank = true;
					}
				}
				if (targetFocusingTank) continue;
				int priority = getCreaturePriority(unit);
				if (priority == 999) continue;
				float dist = unit->position.DistanceTo(localPlayer->position);
				if (target == NULL || priority > maxPriority || (priority == maxPriority && dist < minDist)) {
					target = unit;
					maxPriority = priority;
					minDist = dist;
				}
			}
		}
		if (target != NULL) {
			if (target->Guid != localPlayer->targetGuid) {
				localPlayer->SetTarget(target->Guid);
				return true;
			}
		}
		else {
			bool targetFocusingTank = (targetUnit != NULL && targetUnit->targetGuid == localPlayer->Guid);
			if (targetUnit != NULL) {
				for (int i = 0; i <= NumGroupMembers && !targetFocusingTank; i++) {
					if (GroupMember[i] != NULL && targetUnit->targetGuid == GroupMember[i]->Guid && GroupMember[i]->role == 0) {
						targetFocusingTank = true;
					}
				}
			}
			if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable || targetFocusingTank) {
				return DPSTargeting();
			}
		}
	}
	return false;
}