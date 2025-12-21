#include "../ListAI.h"
#include "../FactionTemplate.h"

void ListAI::DPSTargeting() {
	if ((Leader != NULL) && (Leader->Guid == localPlayer->Guid) && !MCAutoMove) return;
	else if (nbrEnemyPlayer > 0) {
		if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable) {
			WoWUnit* target = NULL; float minDist = INFINITY;
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT && (ListUnits[i].dynamic_flags & DYNAMICFLAG_TAPPEDBYME))
					|| (ListUnits[i].flags & UNIT_FLAG_PLAYER_CONTROLLED)) && ListUnits[i].attackable
					&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID))
					&& (target == NULL || ListUnits[i].position.DistanceTo(target->position) < minDist)) {
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
			if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].attackable && (ListUnits[i].dynamic_flags & DYNAMICFLAG_TAPPEDBYME)
				&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID)) && (target == NULL || ListUnits[i].level < target->level)) {
				target = &ListUnits[i];
			}
		}
		if (target != NULL && target->Guid != localPlayer->targetGuid) {
			localPlayer->SetTarget(target->Guid);
			return;
		}
		for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
			if (HasAggro[i].size() > 0) {
				localPlayer->SetTarget(HasAggro[i][0]->Guid);
				return;
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
				if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].attackable && (ListUnits[i].dynamic_flags & DYNAMICFLAG_TAPPEDBYME)
					&& (ListUnits[i].unitReaction < Neutral || FactionTemplate.isNeutral(ListUnits[i].factionTemplateID)) && (target == NULL || ListUnits[i].level < target->level)) {
					target = &ListUnits[i];
				}
			}
			if (target != NULL) localPlayer->SetTarget(target->Guid);
		}
	}
}