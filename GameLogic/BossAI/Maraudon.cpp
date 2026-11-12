#include "BossAI.h"

bool BossAI::Maraudon() {
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 12222) {
			// Creeping Sludge
			if (Moving != MoveTypes::BossMechanic && localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], MoveTypes::BossMechanic, 20.0f);
				});
				return true;
			}
		}
		else if (ListUnits[i].isdead && ListUnits[i].entryID == 12221) {
			// Noxious Slime
			if (Moving != MoveTypes::BossMechanic && localPlayer->position.DistanceTo(ListUnits[i].position) < 7.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], MoveTypes::BossMechanic, 10.0f);
				});
				return true;
			}
		}
		else if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 12203 && localPlayer->role == 0 && ListUnits[i].targetGuid == localPlayer->Guid && targetUnit && targetUnit->Guid == ListUnits[i].Guid) {
			// Landslide - Tank: Move the boss away
			Position safePosition = Position(354.186188f, -190.702362f, -59.899090f);
			if (localPlayer->position.DistanceTo(safePosition) > 10.0f) {
				ThreadSynchronizer::RunOnMainThread([safePosition]() {
					Functions::MoveTo(safePosition, MoveTypes::BossMechanic);
				});
				return true;
			}
		}
		else if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 13601) {
			// Tinkerer Gizlock
			if (localPlayer->role == 1 && ListUnits[i].targetGuid != localPlayer->Guid && targetUnit && targetUnit->Guid == ListUnits[i].Guid && !localPlayer->isBehind(targetUnit)) {
				// Melee: DPS behind boss
				return BossAI::PositionBehind(targetUnit);
			}
			else if (localPlayer->role == 0 && ListUnits[i].targetGuid == localPlayer->Guid && targetUnit && targetUnit->Guid == ListUnits[i].Guid) {
				// Tank: Move boss in good spot
				Position safePosition = Position(146.348373f, -316.674561f, -174.590485f);
				if (localPlayer->position.DistanceTo(safePosition) > 10.0f) {
					ThreadSynchronizer::RunOnMainThread([safePosition]() {
						Functions::MoveTo(safePosition, MoveTypes::BossMechanic);
						});
					return true;
				}
			}
		}
	}
	return false;
}
