#include "BossAI.h"

static time_t mechanicTimer = time(0);

bool BossAI::ScarletMonastery() {
	for (auto& unit : ListUnits) {
		if (!unit.isInCombatOrEncounter()) continue;
		else if (unit.entryID == 6487) {
			// Arcanist Doan
			ThreadSynchronizer::RunOnMainThread([]() {
				const char* last_yell = (const char*)Functions::GetText("last_yell_monster");
				bool BigAoE = last_yell && std::strcmp(last_yell, "Burn in righteous fire!") == 0;
				if (BigAoE) {
					mechanicTimer = time(0);
					Functions::LuaCall("last_yell_monster = ''");
				}
			});
			if ((float)(time(0) - mechanicTimer) < 10.0f) {
				// Burn in righteous fire!
				Position safePosition = Position(182.921417f, -429.258118f, 18.532364f);
				if (localPlayer->position.DistanceTo(safePosition) > 5.0f) {
					ThreadSynchronizer::RunOnMainThread([safePosition]() {
						Functions::MoveTo(safePosition, MoveTypes::BossMechanic, false, false);
					});
					return true;
				}
			} else if (localPlayer->role == 0 && unit.targetGuid == localPlayer->Guid) {
				// Tank: Move Doan away
				Position bossRoomEdge = Position(132.488647f, -429.408325f, 18.514610f);
				if (localPlayer->position.DistanceTo(bossRoomEdge) > 5.0f) {
					ThreadSynchronizer::RunOnMainThread([bossRoomEdge]() {
						Functions::MoveToFast(bossRoomEdge, MoveTypes::BossMechanic);
					});
					return true;
				}
			}
		}
		else if (unit.entryID == 3975) {
			// Herod
			Position listAwayPoints[] = {
				Position(1945.790161f, -431.490143f, 16.367250f),
				Position(1963.922974f, -411.904755f, 11.704012f),
				Position(1983.241699f, -431.547333f, 11.272270f),
				Position(1966.446167f, -450.624084f, 11.272270f)
			};
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
				if (localPlayer->position.DistanceTo(unit.position) < 15.0f) {
					float maxDist = 0; int pointIndex = 0;
					for (int y = 0; y < 4; y++) {
						float dist = unit.position.DistanceTo(listAwayPoints[y]);
						if (dist > maxDist) {
							pointIndex = y;
							maxDist = dist;
						}
					}
					// Run to the point furthest from Herod
					ThreadSynchronizer::RunOnMainThread([listAwayPoints, pointIndex]() {
						Functions::MoveTo(listAwayPoints[pointIndex], MoveTypes::BossMechanic, false, false);
					});
					return true;
				}
			}
		}
	}
	return false;
}