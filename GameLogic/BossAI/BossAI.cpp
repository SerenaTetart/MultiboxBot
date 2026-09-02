#include "../BossAI.h"

#include <iostream>

bool BossAI::BossAIAction() {
	bool result = false;
	if (!Combat) {
		if (Moving == 10) {
			Moving = 0;
			ThreadSynchronizer::pressKey(0x28);
			ThreadSynchronizer::releaseKey(0x28);
		}
		return false;
	}
	else if (mapID == 189 && localPlayer->zoneID == 796) {
		// Scarlet Armory
		result = BossAI::ScarletMonastery();
	}
	else if (mapID == 349 && localPlayer->zoneID == 2100) {
		// Maraudon
		result = BossAI::Maraudon();
	}

	if (result == false && Moving == 10) {
		Moving = 0;
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	return result;
}