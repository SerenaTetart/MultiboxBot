#include "BossAI.h"
#include "MemoryManager.h"
#include "FunctionsLua.h"

#include <iostream>
#include "Navigation.h"

static time_t mechanicTimer = time(0);

bool BossAI::BossAIAction() {
	//Si une action est à réaliser -> renvoyer True
	//Sinon false
	if (mapID == 189 && localPlayer->zoneID == 796) {
		//Scarlet Armory
		Position listAwayPoints[] = {
			Position(1945.790161f, -431.490143f, 16.367250f),
			Position(1963.922974f, -411.904755f, 11.704012f),
			Position(1983.241699f, -431.547333f, 11.272270f),
			Position(1966.446167f, -450.624084f, 11.272270f)
		};
		bool playedRanged = Functions::PlayerIsRanged();
		for (unsigned int i = 0; i < ListUnits.size(); i++) {
			if ((ListUnits[i].flags & UNIT_FLAG_IN_COMBAT) && strcmp(ListUnits[i].name, "Herod") == 0) {
				// Herod
				ThreadSynchronizer::RunOnMainThread([]() {
					const char* last_yell = (const char*)Functions::GetText("last_yell_monster");
					bool Whirlwind = last_yell && std::strcmp(last_yell, "Blades of light!") == 0;
					if (Whirlwind) {
						mechanicTimer = time(0);
						Functions::LuaCall("last_yell_monster = ''");
					}
				});
				if ((float)(time(0) - mechanicTimer) < 10.0f) {
					// Whirlwind !
					if (localPlayer->position.DistanceTo(ListUnits[i].position) < 15.0f) {
						float maxDist = 0; int pointIndex = 0;
						for (int y = 0; y < 4; y++) {
							float dist = ListUnits[i].position.DistanceTo(listAwayPoints[y]);
							if (dist > maxDist) {
								pointIndex = y;
								maxDist = dist;
							}
						}
						// Run to the point furthest from Herod
						ThreadSynchronizer::RunOnMainThread([listAwayPoints, pointIndex]() {
							Functions::MoveTo(listAwayPoints[pointIndex], 10, false, false);
						});
					}
					else if (Moving == 10) {
						//Stop
						Moving = 0;
						ThreadSynchronizer::pressKey(0x28);
						ThreadSynchronizer::releaseKey(0x28);
					}
					else if (targetUnit != NULL && !IsFacing) {
						ThreadSynchronizer::RunOnMainThread([]() { localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position); });
					}
					return true;
				}
				else return false;
			}
		}
	}
	return false;
}