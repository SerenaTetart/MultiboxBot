#include "../ListAI.h"
#include "../FactionTemplate.h"
#include <iostream>

static int getCreaturePriority(WoWUnit* unit) {
	if (unit->creatureType == Totem) return 0; //Guardian
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

void ListAI::DPSTargeting() {
	if ((Leader != NULL) && (Leader->Guid == localPlayer->Guid) && !MCAutoMove) return;
	else if (nbrEnemyPlayer > 0) {
		if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable) {
			WoWUnit* target = NULL; float minDist = INFINITY;
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (
					(ListUnits[i].flags & UNIT_FLAG_IN_COMBAT || (ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED))
					&& ListUnits[i].attackable
					&& !(ListUnits[i].flags & UNIT_FLAG_POSSESSED)
					&& !ListUnits[i].isdead
					&& !ListUnits[i].isFromGroup
					&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID))
					&& (target == NULL || ListUnits[i].position.DistanceTo(target->position) < minDist)
				) {
					target = &ListUnits[i];
					minDist = ListUnits[i].position.DistanceTo(target->position);
				}
			}
			if (target != NULL) localPlayer->SetTarget(target->Guid);
		}
	}
	else {
		WoWUnit* target = NULL;
		for (unsigned int i = 0; i < ListUnits.size(); i++) {
			if (
				((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) || ListUnits[i].creatureType == Totem)
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
			return;
		}
		else if (targetUnit == NULL || !targetUnit->attackable) {
			for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
				if (HasAggro[i].size() > 0) {
					localPlayer->SetTarget(HasAggro[i][0]->Guid);
					return;
				}
			}
		}
	}
}

void ListAI::TankTargeting() {
	if ((Leader == NULL) || (Leader->Guid != localPlayer->Guid) || MCAutoMove) {
		bool targetFocusingTank = false;
		if (targetUnit != NULL) {
			for (int i = 0; i <= NumGroupMembers; i++) {
				if (GroupMember[i] != NULL && targetUnit->targetGuid == GroupMember[i]->Guid && GroupMember[i]->role == 0) {
					targetFocusingTank = true;
					break;
				}
			}
		}
		if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable || targetFocusingTank) {
			for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
				if (HasAggro[i].size() > 0) {
					localPlayer->SetTarget(HasAggro[i][0]->Guid);
					return;
				}
			}
			WoWUnit* target = NULL;
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (
					(ListUnits[i].flags & UNIT_FLAG_IN_COMBAT)
					&& ListUnits[i].attackable
					&& (ListUnits[i].dynamic_flags & DYNAMICFLAG_TAPPEDBYME)
					&& !(ListUnits[i].flags & UNIT_FLAG_POSSESSED)
					&& !ListUnits[i].isFromGroup
					&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID))
					&& (target == NULL || ListUnits[i].level < target->level)
				) {
					target = &ListUnits[i];
				}
			}
			if (target != NULL) localPlayer->SetTarget(target->Guid);
		}
	}
}