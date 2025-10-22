#include "Game.h"
#include "FunctionsLua.h"
#include "Client.h"
#include "MemoryManager.h"
#include "ListAI.h"

#include "Navigation.h"

#include <iostream>

void MoveToMap(Position targetpos) {
	Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, targetpos);
	if (nextpos.DistanceTo(localPlayer->position) > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
		ThreadSynchronizer::RunOnMainThread([nextpos]() {
			localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
		});
		Moving = 8;
	}
}

void Game::CorpseRun() {
	//Handle all the specific choices of the Leader/Characters
	if (localPlayer->isdead) {
		//Run to the instance
		if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-11207.799805f, 1681.150024f, 60.143227f)) < 5.0f) {
			//Deadmines
			MoveToMap(Position(-11207.504883f, 1680.481934f, 24.358500f));
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-230.988998f, 1571.569946f, 97.390625f)) < 5.0f) {
			//Shadowfang
			MoveToMap(Position(-231.766205f, 1570.990845f, 76.892220f));
		}
		else if (mapID == 1 && localPlayer->corpse_position.DistanceTo(Position(4249.12f, 748.387f, 147.263f)) < 5.0f) {
			//Blackfathom Deeps
			if (localPlayer->zoneID == 719) {
				MoveToMap(Position(4247.740234f, 745.879028f, -24.296673f));
			}
			else {
				MoveToMap(Position(4119.196289f, 888.643982f, 9.724840f));
			}
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-5162.66f, 931.599f, 408.975f)) < 5.0f) {
			//Gnomeregan
			if (localPlayer->position.Z < 280.0f) MoveToMap(Position(-5162.885254f, 928.664246f, 257.180511f));
			else if (localPlayer->position.DistanceTo(Position(-5163.476563f, 659.317871f, 348.278412f)) < 5.0f) {
				ThreadSynchronizer::RunOnMainThread([]() {
					localPlayer->ClickToMove(Move, localPlayer->Guid, Position(-5164.700195f, 649.046021f, 348.270874f));
				});
			}
			else MoveToMap(Position(-5163.476563f, 659.317871f, 348.278412f));
		}
		else {
			Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, localPlayer->corpse_position);
			if (nextpos.DistanceTo(localPlayer->position) > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
				ThreadSynchronizer::RunOnMainThread([nextpos]() {
					localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
				});
				Moving = 8;
			}
		}
	}
}

void Game::TrainSpellRun() {
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12)) {
		//Stormwind
		if (playerClass == "Druid") {
			MoveToMap(Position(-8774.315430f, 1096.036255f, 92.540367f));
		}
		else if (playerClass == "Hunter") {
			MoveToMap(Position(-8417.456055f, 550.952087f, 95.449081f));
		}
		else if (playerClass == "Mage") {
			MoveToMap(Position(-9014.357422f, 873.637695f, 148.616272f));
		}
		else if (playerClass == "Paladin") {
			MoveToMap(Position(-8571.522461f, 863.083008f, 106.518623f));
		}
		else if (playerClass == "Priest") {
			MoveToMap(Position(-8516.106445f, 859.622742f, 109.844681f));
		}
		else if (playerClass == "Rogue") {
			MoveToMap(Position(-8751.646484f, 380.214447f, 101.067060f));
		}
		else if (playerClass == "Warlock") {
			MoveToMap(Position(-8973.632813f, 1033.069702f, 101.404121f));
		}
		else if (playerClass == "Warrior") {
			MoveToMap(Position(-8686.984375f, 323.889099f, 109.437485f));
		}
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1)) {
		//Ironforge
		if (playerClass == "Hunter") {
			MoveToMap(Position(-5012.019043f, -1273.012085f, 507.753845f));
		}
		else if (playerClass == "Mage") {
			MoveToMap(Position(-4613.083008f, -927.388550f, 501.068207f));
		}
		else if (playerClass == "Paladin") {
			MoveToMap(Position(-4601.828613f, -897.461487f, 502.766815f));
		}
		else if (playerClass == "Priest") {
			MoveToMap(Position(-4617.910645f, -907.910706f, 501.070557f));
		}
		else if (playerClass == "Rogue") {
			MoveToMap(Position(-4641.600098f, -1132.239990f, 507.243988f));
		}
		else if (playerClass == "Warlock") {
			MoveToMap(Position(-4602.515137f, -1111.653076f, 504.939484f));
		}
		else if (playerClass == "Warrior") {
			MoveToMap(Position(-5042.364258f, -1243.405273f, 507.754944f));
		}
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141)) {
		//Darnassus
		if (playerClass == "Druid") {
			MoveToMap(Position(10177.738281f, 2582.425537f, 1325.966187f));
		}
		else if (playerClass == "Hunter") {
			MoveToMap(Position(10176.217773f, 2511.905518f, 1342.807129f));
		}
		else if (playerClass == "Priest") {
			MoveToMap(Position(9655.435547f, 2535.975586f, 1331.519043f));
		}
		else if (playerClass == "Rogue") {
			MoveToMap(Position(10084.106445f, 2548.979492f, 1294.893555f));
		}
		else if (playerClass == "Warrior") {
			MoveToMap(Position(9940.208008f, 2282.287842f, 1341.394409f));
		}
	}
}
