#include "BossAI.h"

#include <iostream>

bool BossAI::BossAIAction() {
	if (mapID == 189 && localPlayer->zoneID == 796) {
		// Scarlet Armory
		return BossAI::ScarletMonastery();
	}
	else if (mapID == 349 && localPlayer->zoneID == 2100) {
		// Maraudon
		return BossAI::Maraudon();
	}
	return false;
}