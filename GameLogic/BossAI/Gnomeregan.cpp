#include "BossAI.h"

bool BossAI::Gnomeregan() {
	for (auto& unit : ListUnits) {
		if (unit.entryID == 7915 || unit.entryID == 8035) {
			// Walking Bomb || Dark Iron Land Mine
			unit.isEncounterUnit = true;
		}
		else if (unit.entryID == 6229 && (unit.flags & UNIT_FLAG_IN_COMBAT) && localPlayer->role == 0 && unit.targetGuid == localPlayer->Guid) {
			// Crowd Pummeler 9-60
			// Tank: Move the pummeler away
			Position safePosition = Position(-904.907043f, 329.250092f, -272.596649f);
			if (localPlayer->position.DistanceTo(safePosition) > 10.0f) {
				ThreadSynchronizer::RunOnMainThread([safePosition]() {
					Functions::MoveTo(safePosition, MoveTypes::BossMechanic);
				});
				return true;
			}
		}
	}
	return false;
}