#include "BossAI.h"
#include <iostream>

bool BossAI::Uldaman() {
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if (ListUnits[i].entryID == 7226) {
			// Sand Storm
			if (Moving != MoveTypes::BossMechanic && localPlayer->position.DistanceTo(ListUnits[i].position) < 7.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], MoveTypes::BossMechanic, 10.0f);
				});
				return true;
			}
		}
	}
	return false;
}