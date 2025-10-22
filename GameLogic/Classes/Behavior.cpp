#include "../ListAI.h"

void ListAI::DPSTargeting() {
	if ((Leader != NULL) && (Leader->Guid == localPlayer->Guid) && !MCAutoMove) return;
	else if (PvPTarget != NULL) localPlayer->SetTarget(PvPTarget->Guid);
	else {
		bool tankInGrp = false;
		for (int i = 1; i <= NumGroupMembers; i++) {
			if (GroupMember[i] != NULL && GroupMember[i]->role == 0) tankInGrp = true;
		}
		if (tankInGrp && (Combat || Leader == NULL || Leader->Guid != localPlayer->Guid)) {
			bool targetFocusingTank = false;
			if (targetUnit != NULL && !targetUnit->isdead && targetUnit->attackable) {
				for (int i = 1; i <= NumGroupMembers; i++) {
					if (GroupMember[i] != NULL && targetUnit->targetGuid == GroupMember[i]->Guid && GroupMember[i]->role == 0) {
						targetFocusingTank = true;
						return;
					}
				}
			}
			if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable || !targetFocusingTank) {
				for (int i = 1; i <= NumGroupMembers; i++) {
					if (GroupMember[i] != NULL && GroupMember[i]->targetGuid != NULL && GroupMember[i]->role == 0) {
						localPlayer->SetTarget(GroupMember[i]->targetGuid);
						return;
					}
				}
				for (int y = 0; y < 4; y++) {
					for (int i = NumGroupMembers; i >= 0; i--) {
						if (GroupMember[i] != NULL && GroupMember[i]->role == y && HasAggro[i].size() > 0) {
							localPlayer->SetTarget(HasAggro[i][0]->Guid);
							return;
						}
					}
				}
			}
		}
		else {
			for (int y = 0; y < 4; y++) {
				for (int i = NumGroupMembers; i >= 0; i--) {
					if (GroupMember[i] != NULL && GroupMember[i]->role == y && HasAggro[i].size() > 0) {
						localPlayer->SetTarget(HasAggro[i][0]->Guid);
						return;
					}
				}
			}
		}
	}
}

void ListAI::TankTargeting() {
	if ((Leader == NULL) || (Leader->Guid != localPlayer->Guid) || MCAutoMove) {
		bool targetFocusingTank = false;
		if (PvPTarget != NULL) {
			localPlayer->SetTarget(PvPTarget->Guid);
			return;
		}
		else if (targetUnit != NULL) {
			for (int i = 0; i <= NumGroupMembers; i++) {
				if (GroupMember[i] != NULL && targetUnit->targetGuid == GroupMember[i]->Guid && GroupMember[i]->role == 0) {
					targetFocusingTank = true;
					break;
				}
			}
		}
		if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable || targetFocusingTank) {
			if (TankTarget != NULL) {
				localPlayer->SetTarget(TankTarget->Guid);
				return;
			}
			else {
				for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
					if (HasAggro[i].size() > 0) {
						localPlayer->SetTarget(HasAggro[i][0]->Guid);
						return;
					}
				}
			}
		}
	}
}