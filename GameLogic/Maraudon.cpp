#include "BossAI.h"
#include "MemoryManager.h"
#include "FunctionsLua.h"
#include <time.h>

bool BossAI::Maraudon() {
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 12222) {
			// Creeping Sludge
			bool isRanged = Functions::PlayerIsRanged();
			if (localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], 10);
				});
				return true;
			}
			else if (Moving == 10 || (localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
				//Stop
				Moving = 0;
				ThreadSynchronizer::pressKey(0x28);
				ThreadSynchronizer::releaseKey(0x28);
			}
			if (!isRanged) return true;
		}
		else if (ListUnits[i].isdead && ListUnits[i].entryID == 12221) {
			// Noxious Slime
			if (localPlayer->position.DistanceTo(ListUnits[i].position) < 7.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], 10);
				});
				return true;
			}
		}
	}
	return false;
}