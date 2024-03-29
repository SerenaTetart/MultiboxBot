#include "Game.h"
#include "Functions.h"
#include "Client.h"
#include "MemoryManager.h"
#include "ListAI.h"

#include <iostream>
//#include <chrono>

static unsigned long long playerGuid = 0;
static unsigned long long pastPlayerGuid = 0;
static std::string playerName = "";

void Game::MainLoop() {
	while (Client::client_running == true) {
		if (Client::bot_running == false) {
			ThreadSynchronizer::RunOnMainThread(
				[]() {
					playerGuid = Functions::GetPlayerGuid();
					if (playerGuid > 0) {
						if (keyTarget && Leader != NULL) {
							std::string msg = "AssistByName('" + (std::string)Leader->name + "')";
							Functions::LuaCall(msg.c_str());
							keyTarget = false;
						}
						else if (keyHearthstone) {
							Functions::UseItem("Hearthstone");
							keyHearthstone = false;
						}
						else if (keyMount) {
							Functions::UseItem("Bridle");
							keyMount = false;
						}
						if (playerGuid != pastPlayerGuid) {
							pastPlayerGuid = playerGuid;
							playerClass = Functions::UnitClass("player");
							playerName = Functions::UnitName("player");
							std::string msg = ("Name " + playerName + " Class " + playerClass);
							Client::sendMessage(msg);
						}
					}
				}
			);
		}

		while (Client::bot_running == true) {

			// ========================================== //
			// ===========   Initialisation   =========== //
			// ========================================== //

			//auto start = std::chrono::high_resolution_clock::now();

			ThreadSynchronizer::RunOnMainThread(
				[]() {
					playerGuid = Functions::GetPlayerGuid();
					if (playerGuid > 0) {
						if (Functions::IsInRaid()) tarType = "raid";
						else tarType = "party";
						NumGroupMembers = Functions::GetNumGroupMembers();
						IsInGroup = Functions::IsInGroup();

						Functions::EnumerateVisibleObjects(0);

						if (localPlayer != NULL) targetUnit = localPlayer->getTarget();

						if (Functions::GetRepairAllCost() > 0) Functions::LuaCall("RepairAllItems()");
						if (Functions::GetMerchantNumItems() > 0) Functions::SellUselessItems();

						if (keyTarget && Leader != NULL) {
							std::string msg = "AssistByName('" + (std::string)Leader->name + "')";
							Functions::LuaCall(msg.c_str());
							keyTarget = false;
						}
						else if (keyHearthstone) {
							Functions::UseItem("Hearthstone");
							keyHearthstone = false;
						}
						else if (keyMount) {
							Functions::UseItem("Bridle");
							keyMount = false;
						}

						if (playerGuid != pastPlayerGuid) {
							pastPlayerGuid = playerGuid;
							playerClass = Functions::UnitClass("player");
							playerName = Functions::UnitName("player");
							std::string msg = ("Name " + playerName + " Class " + playerClass);
							Client::sendMessage(msg);
						}
					}
				}
			);

			if (playerGuid > 0) { //in-game
				if (localPlayer != NULL) {

					Functions::ClassifyHeal();
					std::tie(nbrEnemy, nbrCloseEnemy, nbrCloseEnemyFacing, nbrEnemyPlayer) = Functions::countEnemies();

					IsFacing = false;
					if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) IsFacing = localPlayer->isFacing(targetUnit->position, 0.4f);

					distTarget = 0;
					if (targetUnit != NULL) distTarget = localPlayer->position.DistanceTo(targetUnit->position);

					Combat = (localPlayer->flags & UNIT_FLAG_IN_COMBAT) == UNIT_FLAG_IN_COMBAT;

					hasTargetAggro = false;
					if (targetUnit != NULL) {
						for (unsigned int i = 0; i < HasAggro[0].size(); i++) {
							if (targetUnit->Guid == HasAggro[0][i]->Guid) hasTargetAggro = true;
						}
					}

					if (playerGuid > 0 && playerGuid != pastPlayerGuid) {
						pastPlayerGuid = playerGuid;
						std::string msg = ("Name " + playerName + " Class " + playerClass);
						Client::sendMessage(msg);
					}
				} else std::cout << "localPlayer NULL !\n";

				/*auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> float_ms = end - start;
				std::cout << "Initialisation: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
				
				// ========================================== //
				// ==============   Movements   ============= //
				// ========================================== //

				//start = std::chrono::high_resolution_clock::now();

				if (localPlayer != NULL) {
					enemy_close = false;
					Position back_pos = Position((cos(localPlayer->facing + (2 * halfPI)) * 2.0f) + localPlayer->position.X
						, (sin(localPlayer->facing + (2 * halfPI)) * 2.0f) + localPlayer->position.Y
						, localPlayer->position.Z);
					ThreadSynchronizer::RunOnMainThread([=]() {
						enemy_close = Functions::EnemyClose(localPlayer->position);
						obstacle_back = Functions::Intersect(localPlayer->position, back_pos, 2.00f)
							|| (((localPlayer->movement_flags & MOVEFLAG_SWIMMING) != MOVEFLAG_SWIMMING)
								&& ((localPlayer->movement_flags & MOVEFLAG_WATERWALKING) != MOVEFLAG_WATERWALKING)
								&& Functions::GetDepth(back_pos, 2.00f) > 2.00f);
						los_target = true;
						if (targetUnit != NULL) {
							los_target = !Functions::Intersect(localPlayer->position, targetUnit->position, 2.00f);
							if (IsInGroup && (Leader != NULL) && (Leader->Guid != localPlayer->Guid) && targetUnit->attackable && !targetUnit->isdead
								&& !(targetUnit->flags & UNIT_FLAG_IN_COMBAT) && (targetUnit->Guid != Leader->targetGuid))
								Functions::LuaCall("ClearTarget()");
						}
						hasDrink = Functions::HasDrink();
					});
					bool playerIsRanged = Functions::PlayerIsRanged();
					int drinkingIDs[15] = { 430, 431, 432, 1133, 1135, 1137, 24355, 25696, 26261, 26402, 26473, 26475, 29007, 10250, 22734 };
					int RaptorStrikeIDs[8] = { 2973, 14260, 14261, 14262, 14263, 14264, 14265, 14266 };
					if (IsSitting && ((localPlayer->prctMana > 85) || Combat || !localPlayer->hasBuff(drinkingIDs, 15) || (localPlayer->speed > 0))) {
						//Stop sitting
						IsSitting = false;
						Functions::pressKey(0x28);
						Functions::releaseKey(0x28);
						Moving = 0;
					}
					else if (!Combat && !IsSitting && (localPlayer->speed == 0) && (Moving == 0 || Moving == 4) && (localPlayer->movement_flags == MOVEFLAG_NONE) && (localPlayer->prctMana < 50) && (hasDrink > 0)) {
						//Drink
						IsSitting = true;
						ThreadSynchronizer::RunOnMainThread([]() { Functions::UseItem(hasDrink); });
					}
					else if (!passiveGroup && (targetUnit != NULL) && targetUnit->attackable && !targetUnit->isdead && (((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0)) || playerClass == "Hunter")) {
						if (playerIsRanged) {
							if ((Moving == 4 || Moving == 2 || Moving == 5) && distTarget < 30.0f) {
								//Running and (target < 30 yard) => stop
								Functions::pressKey(0x28);
								Functions::releaseKey(0x28);
								Moving = 0;
							}
							else if ((Moving == 0 || (Moving == 6 && localPlayer->speed == 0)) && !los_target) {
								//!LoS => Find LoS
								ThreadSynchronizer::RunOnMainThread([]() {
									Functions::MoveLoS(targetUnit->position);
								});
							}
							else if (distTarget > 30.0f && !IsSitting && (Moving == 0 || Moving == 2 || Moving == 4 || (Moving == 6 && localPlayer->speed == 0))) {
								//Target > 30 yard => Run to it
								bool swimming = ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) || (localPlayer->movement_flags & MOVEFLAG_WATERWALKING)
									|| (targetUnit->movement_flags & MOVEFLAG_SWIMMING) || (targetUnit->movement_flags & MOVEFLAG_WATERWALKING));
								if (swimming) {
									ThreadSynchronizer::RunOnMainThread([]() {
										localPlayer->ClickToMove(Move, targetUnit->Guid, targetUnit->position);
									});
									Moving = 2;
								}
								else {
									ThreadSynchronizer::RunOnMainThread([]() {
										Functions::MoveObstacle(targetUnit->position);
									});
									Moving = 2;
								}
							}
							else if (Moving == 6 && los_target) {
								//Looking for LoS, found it => stop
								Functions::pressKey(0x28);
								Functions::releaseKey(0x28);
								Moving = 0;
							}
							else if (Moving == 3 && (distTarget > 12.0f || obstacle_back || enemy_close
								|| ((!(targetUnit->flags & UNIT_FLAG_STUNNED) && !(targetUnit->movement_flags & MOVEFLAG_ROOT))
									&& ((!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && hasTargetAggro)
										|| ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed > 4.5))))) {
								//Walking backward and (target > 12 yard || Creature aggro || target running)
								Functions::releaseKey(0x28);
								Moving = 0;
							}
							else if ((Moving == 0) && !IsFacing) {
								//Nothing to do => face target
								ThreadSynchronizer::RunOnMainThread([]() { localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position); });
							}
							else if ((Moving == 0 || Moving == 3) && distTarget < 12.0f && !obstacle_back && !enemy_close
								&& ((targetUnit->flags & UNIT_FLAG_STUNNED) || (targetUnit->movement_flags & MOVEFLAG_ROOT)
									|| (!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && !hasTargetAggro)
									|| ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed <= 4.5 && targetUnit->speed > 0))) {
								//(Creature not aggro || Player slowed) && < 12 yard => Walk backward
								Functions::pressKey(0x28);
								Moving = 3;
							}
						} 
						else if(Leader == NULL || ((std::string) Leader->name != playerName) || tankAutoMove) {
							if ((Moving == 0 || (Moving == 6 && localPlayer->speed == 0)) && !los_target) {
								//Find LoS
								ThreadSynchronizer::RunOnMainThread([]() {
									Functions::MoveLoS(targetUnit->position);
								});
							}
							else if (distTarget > 5.0f && !IsSitting && (Moving == 0 || Moving == 2 || Moving == 4 || (Moving == 6 && localPlayer->speed == 0))) {
								//Target > 5 yard => Run to it
								bool swimming = ((localPlayer->movement_flags & MOVEFLAG_SWIMMING) || (localPlayer->movement_flags & MOVEFLAG_WATERWALKING)
									|| (targetUnit->movement_flags & MOVEFLAG_SWIMMING) || (targetUnit->movement_flags & MOVEFLAG_WATERWALKING));
								if (swimming) {
									ThreadSynchronizer::RunOnMainThread([]() {
										localPlayer->ClickToMove(Move, targetUnit->Guid, targetUnit->position);
									});
									Moving = 2;
								}
								else {
									ThreadSynchronizer::RunOnMainThread([]() {
										Functions::MoveObstacle(targetUnit->position);
									});
									Moving = 2;
								}
							}
							else if (Moving == 6 && los_target) {
								//Looking for LoS, found it => stop
								Functions::pressKey(0x28);
								Functions::releaseKey(0x28);
								Moving = 0;
							}
							else if (Moving == 2 || Moving == 4 || Moving == 5) {
								Functions::pressKey(0x28);
								Functions::releaseKey(0x28);
								Moving = 0;
							}
							else if ((Moving == 0) && !IsFacing) ThreadSynchronizer::RunOnMainThread([]() { localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position); });
						}
					}
					else if (Moving == 1 || Moving == 2 || Moving == 6) {
						Functions::pressKey(0x28);
						Functions::releaseKey(0x28);
						Moving = 0;
					}
					else if ((Moving == 5 && localPlayer->speed == 0) && !los_target && (targetUnit != NULL) && (targetUnit->unitReaction >= Friendly)) {
						//Find LoS (ally)
						ThreadSynchronizer::RunOnMainThread([]() {
							Functions::MoveLoS(targetUnit->position);
							Moving = 5;
						});
					}
					else if (Moving == 3) {
						Functions::releaseKey(0x28);
						Moving = 0;
					}
					else if (Moving == 5 && (targetUnit == NULL || (targetUnit->unitReaction < Friendly) || ((targetUnit->unitReaction >= Friendly) && los_target))) {
						if((localPlayer->speed > 0)) {
							Functions::pressKey(0x28);
							Functions::releaseKey(0x28);
						}
						Moving = 0;
					}
					else if ((Leader != NULL) && (Leader->Guid != localPlayer->Guid) && (!Combat || passiveGroup) && !IsSitting && IsInGroup && (localPlayer->castInfo == 0)
					&& (localPlayer->channelInfo == 0) && (targetUnit == NULL || !targetUnit->attackable || targetUnit->isdead || passiveGroup)) {
						//Follow
						Functions::FollowMultibox(positionCircle);
						Moving = 4;
					}
				}
				/*end = std::chrono::high_resolution_clock::now();
				float_ms = end - start;
				std::cout << "Movement: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
				
				// ========================================== //
				// ===============   Actions   ============== //
				// ========================================== //

				//start = std::chrono::high_resolution_clock::now();

				if (localPlayer != NULL && !IsSitting) {
					if (playerClass == "Druid" && playerSpec == 0) ListAI::DruidBalance();
					else if (playerClass == "Hunter") ListAI::HunterDps();
					else if (playerClass == "Mage") ListAI::MageDps();
					else if (playerClass == "Paladin" && playerSpec == 0) ListAI::PaladinHeal();
					else if (playerClass == "Paladin" && playerSpec == 1) ListAI::PaladinTank();
					else if (playerClass == "Paladin") ListAI::PaladinDps();
					else if (playerClass == "Priest") ListAI::PriestHeal();
					else if (playerClass == "Rogue") ListAI::RogueDps();
					else if (playerClass == "Warlock") ListAI::WarlockDps();
					else if (playerClass == "Warrior" && playerSpec == 2) ListAI::WarriorTank();
				}

				/*end = std::chrono::high_resolution_clock::now();
				float_ms = end - start;
				std::cout << "Actions: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
			}
			Sleep(250);
		}
		if (Moving > 0) {
			if (Moving < 3) Functions::pressKey(0x28);
			Functions::releaseKey(0x28);
			Moving = 0;
		}
		Sleep(250);
	}
}

std::vector<WoWUnit *> HasAggro[40];
bool Combat = false, IsSitting = false, IsInGroup = false, IsFacing = false, hasTargetAggro = false, tankAutoFocus = false, tankAutoMove = false,
	keyTarget = false, keyHearthstone = false, keyMount = false, los_target = false, obstacle_back = false, obstacle_front = false, passiveGroup = false, petAttacking = false,
	enemy_close = false;
int AoEHeal = 0, nbrEnemy = 0, nbrCloseEnemy = 0, nbrCloseEnemyFacing = 0, nbrEnemyPlayer = 0, Moving = 0, NumGroupMembers = 0, playerSpec = 0, positionCircle = 0, hasDrink = 0;
unsigned int LastTarget = 0;
float distTarget = 0, halfPI = acosf(0);
std::string tarType = "party", playerClass = "null";
std::vector<std::tuple<std::string, int, int>> leaderInfos;
std::vector<int> HealTargetArray;
std::vector<Position> path;
WoWUnit* ccTarget = NULL; WoWUnit* targetUnit = NULL; WoWUnit* GroupMember[40]; WoWUnit* Leader = NULL;
time_t current_time = time(0);