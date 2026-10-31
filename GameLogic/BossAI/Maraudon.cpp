#include "BossAI.h"

bool BossAI::Maraudon() {
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 12222) {
			// Creeping Sludge
			if (Moving != MoveTypes::BossMechanic && localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], MoveTypes::BossMechanic, 15.0f);
				});
				return true;
			}
		}
		else if (ListUnits[i].isdead && ListUnits[i].entryID == 12221) {
			// Noxious Slime
			if (Moving != MoveTypes::BossMechanic && localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], MoveTypes::BossMechanic, 10.0f);
				});
				return true;
			}
		}
	}
	return false;
}