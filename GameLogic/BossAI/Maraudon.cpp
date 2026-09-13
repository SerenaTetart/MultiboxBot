#include "../BossAI.h"

bool BossAI::Maraudon() {
	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && ListUnits[i].entryID == 12222) {
			// Creeping Sludge
			Navigation::AddBlacklist(mapID, "creeping_sludge_" + std::to_string(ListUnits[i].Guid), ListUnits[i].position, 10.0f, 55);
			if (Moving != 10 && localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], 10, 15.0f);
				});
				return true;
			}
		}
		else if (ListUnits[i].isdead && ListUnits[i].entryID == 12221) {
			// Noxious Slime
			Navigation::AddBlacklist(mapID, "noxious_slime_" + std::to_string(ListUnits[i].Guid), ListUnits[i].position, 7.0f, 55);
			if (Moving != 10 && localPlayer->position.DistanceTo(ListUnits[i].position) < 10.0f) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::StepBack(&ListUnits[i], 10, 15.0f);
				});
				return true;
			}
		}
	}
	return false;
}