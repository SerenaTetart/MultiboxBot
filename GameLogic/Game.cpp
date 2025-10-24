#include "Game.h"
#include "FunctionsLua.h"
#include "Client.h"
#include "MemoryManager.h"
#include "ListAI.h"

#include "Navigation.h"

#include <iostream>
#include <chrono>

static unsigned long long playerGuid = 0;
static unsigned long long pastPlayerGuid = 0;

void Game::MainLoop() {

	while (Client::client_running == true) {

		ThreadSynchronizer::RunOnMainThread(
			[]() {
				playerGuid = Functions::GetPlayerGuid();
				if (playerGuid > 0) {
					NumGroupMembers = FunctionsLua::GetNumGroupMembers();
					tarType = "party";
					if (NumGroupMembers > 5) {
					        IsInGroup = 2;
					        tarType = "raid";
					}
					else if (NumGroupMembers > 0) IsInGroup = 1;
					else IsInGroup = 0;

					if (playerGuid != pastPlayerGuid) {
						
						Functions::EnumerateVisibleObjects(0);

						pastPlayerGuid = playerGuid;

						if (localPlayer != NULL) {
							std::string msg = ("Name " + FunctionsLua::UnitName("player") + " Class " + localPlayer->className);
							Client::sendMessage(msg);

							std::string listSkills[] = { "Skinning", "Mining", "Herbalism", "Tailoring", "Leatherworking", "Blacksmithing", "Enchanting", "Alchemy", "Engineering" };
							int skills[] = { 0, 0 };
							std::tie(skills[0], skills[1]) = FunctionsLua::GetTradeSkillList(listSkills, 8);
							msg = ("Craft" + std::to_string(skills[0]) + std::to_string(skills[1]));
							Client::sendMessage(msg);
						}
					}
				}
			}
		);

		while (Client::bot_running == true && (Leader == NULL || (Leader->Guid != playerGuid) || !MCNoAuto)) {
			//std::cout << "Preprocessing\n";
			ThreadSynchronizer::RunOnMainThread(
				[]() {
					playerGuid = Functions::GetPlayerGuid();
					if (playerGuid > 0) {
						NumGroupMembers = FunctionsLua::GetNumGroupMembers();
						tarType = "party";
					        if (NumGroupMembers > 5) {
					                IsInGroup = 2;
					                tarType = "raid";
					        }
					        else if (NumGroupMembers > 0) IsInGroup = 1;
					        else IsInGroup = 0;
						mapID = Functions::GetMapID();

						Functions::EnumerateVisibleObjects(0);

						if (localPlayer == NULL || localPlayer->name == "") pastPlayerGuid = 0;
						else pastPlayerGuid = playerGuid;
						
						if (localPlayer != NULL) {
							targetUnit = localPlayer->getTarget();

							FunctionsLua::MakeVirtualInventory(&virtualInventory);

							if (FunctionsLua::GetRepairAllCost() > 0) Functions::LuaCall("RepairAllItems()");
							if (FunctionsLua::GetMerchantNumItems() > 0) FunctionsLua::SellUselessItems();

							if (keybindTrigger == 1) {
								FunctionsLua::UseItem(6948);
								keybindTrigger = 0;
							}
							else if (keybindTrigger == 2) {
								//FunctionsLua::UseItem("Bridle");
								keybindTrigger = 0;
							}

							if (playerGuid != pastPlayerGuid) {
								pastPlayerGuid = playerGuid;
								std::string msg = ("Name " + FunctionsLua::UnitName("player") + " Class " + localPlayer->className);
								Client::sendMessage(msg);
							}

							skinningLevel = FunctionsLua::GetTradingSkill("Skinning");
							miningLevel = FunctionsLua::GetTradingSkill("Mining");
							herbalismLevel = FunctionsLua::GetTradingSkill("Herbalism");

							if (FunctionsLua::SpellIsTargeting()) FunctionsLua::SpellStopTargeting();

							Functions::LuaCall("AcceptTrade()");
						}
					}
				}
			);

			// ========================================== //
			// ===========   Initialisation   =========== //
			// ========================================== //

			/*auto start = std::chrono::high_resolution_clock::now();
			std::cout << "Initialisation start\n";*/

			if (localPlayer == NULL) {
				Sleep(333);
				break;
			}

			Functions::ClassifyHeal();
			std::tie(nbrEnemy, nbrCloseEnemy, nbrCloseEnemyFacing, nbrEnemyPlayer) = Functions::countEnemies();

			Combat = (localPlayer->flags & UNIT_FLAG_IN_COMBAT) == UNIT_FLAG_IN_COMBAT;

			IsFacing = false; distTarget = 0; hasTargetAggro = false;
			if(targetUnit != NULL) {
				if(targetUnit->attackable) IsFacing = localPlayer->isFacing(targetUnit->position, 0.3f);
				distTarget = localPlayer->position.DistanceTo(targetUnit->position);

				for (unsigned int i = 0; i < HasAggro[0].size(); i++) {
					if (targetUnit->Guid == HasAggro[0][i]->Guid) hasTargetAggro = true;
				}
			}

			/*auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> float_ms = end - start;
			std::cout << "Initialisation: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
				
			// ========================================== //
			// ==============   Movements   ============= //
			// ========================================== //

			//start = std::chrono::high_resolution_clock::now();


			int RaptorStrikeIDs[8] = { 2973, 14260, 14261, 14262, 14263, 14264, 14265, 14266 };
			int HeroicStrikeIDs[9] = { 78, 284, 285, 1608, 11564, 11565, 11566, 11567, 25286 };
			int CleaveIDs[5] = { 845, 7369, 11608, 11609, 20569 };
			if (localPlayer != NULL && (localPlayer->channelInfo == 0) && (localPlayer->castInfo == 0 || localPlayer->isCasting(RaptorStrikeIDs, 8) || localPlayer->isCasting(HeroicStrikeIDs, 9) || localPlayer->isCasting(CleaveIDs, 5))) {
				float halfPI = acosf(0);
				Position back_pos = Position((cos(localPlayer->facing + (2 * halfPI)) * 2.0f) + localPlayer->position.X
					, (sin(localPlayer->facing + (2 * halfPI)) * 2.0f) + localPlayer->position.Y
					, localPlayer->position.Z);
				ThreadSynchronizer::RunOnMainThread([=]() {
					los_target = true;
					if (targetUnit != NULL) {
						los_target = !Functions::Intersect(localPlayer->position, targetUnit->position);
						if (IsInGroup && (Leader != NULL) && (Leader->Guid != localPlayer->Guid) && targetUnit->attackable
							&& !(targetUnit->flags & UNIT_FLAG_IN_COMBAT) && (targetUnit->Guid != Leader->targetGuid))
							Functions::LuaCall("ClearTarget()");
					}
					if (IsFacing && distTarget < 5.0f && (float(time(0) - autoAttackCD) >= FunctionsLua::UnitAttackSpeed("player")) && FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack")))
						autoAttackCD = time(0);
				});
					//Reset Loot History after 20 sec
				unsigned int ind = 0;
				while (ind < LootHistory.size()) {
					if (float(time(0) - get<1>(LootHistory[ind])) >= 20.0f) LootHistory.erase(LootHistory.begin() + ind);
					else ind++;
				}
				bool playerIsRanged = Functions::PlayerIsRanged();
				int drinkingIDs[15] = { 430, 431, 432, 1133, 1135, 1137, 24355, 25696, 26261, 26402, 26473, 26475, 29007, 10250, 22734 };
				if (localPlayer->isdead && localPlayer->getHealth() == 1) {
					//Logic actions
					CorpseRun();
				}
				else if (IsSitting && ((localPlayer->prctMana > 85) || Combat || !localPlayer->hasBuff(drinkingIDs, 15) || (localPlayer->speed > 0))) {
					//Stop sitting
					IsSitting = false;
					ThreadSynchronizer::pressKey(0x28);
					ThreadSynchronizer::releaseKey(0x28);
					Moving = 0;
				}
				else if (!Combat && !IsSitting && (localPlayer->channelInfo == 0) && (localPlayer->castInfo == 0) && !localPlayer->isMoving && (Moving == 0 || Moving == 4) && (localPlayer->movement_flags == MOVEFLAG_NONE) && (localPlayer->prctMana < 50 && localPlayer->prctMana > 0) && (FunctionsLua::HasDrink() > 0)) {
					//Drink
					IsSitting = true;
					ThreadSynchronizer::RunOnMainThread([]() { FunctionsLua::UseItem(FunctionsLua::HasDrink()); });
				}
				else if (!Combat && autoLearnSpells) {
					// Go learn spells
					TrainSpellRun();
				}
				else if (!passiveGroup && (Leader == NULL || (Leader->Guid != localPlayer->Guid) || MCAutoMove) && (targetUnit != NULL) && targetUnit->attackable && !targetUnit->isdead) {
					if (playerIsRanged) {
						if ((Moving == 4 || Moving == 2 || Moving == 5) && distTarget < 30.0f) {
							//Running and (target < 30 yard) => stop
							ThreadSynchronizer::pressKey(0x28);
							ThreadSynchronizer::releaseKey(0x28);
							Moving = 0;
						}
						else if ((Moving == 0 || (Moving == 6 && localPlayer->speed == 0)) && !los_target) {
							//!LoS => Find LoS
							ThreadSynchronizer::RunOnMainThread([]() {
								Functions::MoveToLoS(targetUnit->position, 6);
							});
						}
						else if (distTarget > 30.0f && !IsSitting && (Moving == 0 || Moving == 2 || Moving == 4 || (Moving == 6 && localPlayer->speed == 0))) {
							//Target > 30 yard => Run to it
							bool targetSwim = false; if (targetUnit->movement_flags & MOVEFLAG_SWIMMING) targetSwim = true;
							ThreadSynchronizer::RunOnMainThread([=]() {
								Functions::MoveTo(targetUnit->position, 2, true, targetSwim);
							});
						}
						else if (Moving == 6 && los_target) {
							//Looking for LoS, found it => stop
							ThreadSynchronizer::pressKey(0x28);
							ThreadSynchronizer::releaseKey(0x28);
							Moving = 0;
						}
						else if (Moving == 3 && (distTarget > 12.0f || ((!(targetUnit->flags & UNIT_FLAG_STUNNED) && !(targetUnit->movement_flags & MOVEFLAG_ROOT))
								&& ((!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && hasTargetAggro) || ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed > 4.5))))) {
							//Walking backward and (target > 12 yard || Creature aggro || target running)
							ThreadSynchronizer::pressKey(0x28);
							ThreadSynchronizer::releaseKey(0x28);
							Moving = 0;
						}
						else if ((Moving == 0 || Moving == 3) && distTarget < 12.0f
							&& ((targetUnit->flags & UNIT_FLAG_STUNNED) || (targetUnit->movement_flags & MOVEFLAG_ROOT)
							|| (!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && !hasTargetAggro)
							|| ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed <= 4.5 && targetUnit->speed > 0))) {
							//(Creature not aggro || Player slowed) && < 12 yard => Walk backward
							ThreadSynchronizer::RunOnMainThread([]() {
								if (Functions::StepBack(targetUnit->position, 3) == false) {
									localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position);
								}
							});
						}
						else if ((Moving == 0) && !IsFacing) {
							//Nothing to do => face target
							ThreadSynchronizer::RunOnMainThread([]() { localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position); });
						}
					}
					else {
						if (distTarget > 5.0f && !IsSitting && (Moving == 0 || Moving == 2 || Moving == 4 || (Moving == 6 && localPlayer->speed == 0))) {
							//Target > 5 yard => Run to it
							bool targetSwim = false; if (targetUnit->movement_flags & MOVEFLAG_SWIMMING) targetSwim = true;
							ThreadSynchronizer::RunOnMainThread([=]() {
								Functions::MoveTo(targetUnit->position, 2, false, targetSwim);
							});
						}
						else if (Moving == 0 && !IsFacing) ThreadSynchronizer::RunOnMainThread([]() { localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position); });
						else if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
							ThreadSynchronizer::pressKey(0x28);
							ThreadSynchronizer::releaseKey(0x28);
							Moving = 0;
						}
						else Moving = 0;
					}
				}
				else if ((Moving == 5 && localPlayer->speed == 0) && !los_target && (targetUnit != NULL) && (targetUnit->unitReaction >= Friendly)) {
					//Find LoS (ally)
					ThreadSynchronizer::RunOnMainThread([=]() {
						Functions::MoveToLoS(targetUnit->position, 5);
					});
				}
				else if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) && Moving != 5 && Moving != 4 && Moving != 0) {
					ThreadSynchronizer::pressKey(0x28);
					ThreadSynchronizer::releaseKey(0x28);
					Moving = 0;
				}
				else if (localPlayer->movement_flags & MOVEFLAG_BACKWARD && Moving != 0) {
					ThreadSynchronizer::releaseKey(0x28);
					Moving = 0;
				}
				else if (localPlayer->speed == 0 && Moving != 4 && Moving != 0 && Moving != 5) {
					Moving = 0;
				}
				else if (Moving == 5 && (targetUnit == NULL || (targetUnit->unitReaction < Friendly) || ((targetUnit->unitReaction >= Friendly) && los_target))) {
					if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
						ThreadSynchronizer::pressKey(0x28);
						ThreadSynchronizer::releaseKey(0x28);
					}
					Moving = 0;
				}
				else if ((!Combat || passiveGroup) && (Moving == 0 || Moving == 4) && !localPlayer->isdead && !IsSitting && (float(time(0) - gatheringCD) > 5.5f) && (localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) &&
					(Leader == NULL || Leader->Guid != localPlayer->Guid || MCAutoMove) && (targetUnit == NULL || !targetUnit->attackable || targetUnit->isdead || passiveGroup)) {
					//Loot
					bool looted = false;
					if (herbalismLevel > 0 || miningLevel > 0) {
						for (unsigned int i = 0; i < ListGameObjects.size(); i++) {
							if (ListGameObjects[i].gatherType == 0) continue;
							else if (IsInGroup && Leader != NULL && Leader->position.DistanceTo(ListGameObjects[i].position) > 60.0f)
								continue;
							else if (Functions::enemyClose(ListGameObjects[i].position, 20.0f)) continue;
							int skillLevel = herbalismLevel; if (ListGameObjects[i].gatherType == 1) skillLevel = miningLevel;
							if ((ListGameObjects[i].gatherType == 1 && skillLevel >= ListGameObjects[i].level && skillLevel < ListGameObjects[i].level + 150)
								|| (ListGameObjects[i].gatherType == 2 && skillLevel >= ListGameObjects[i].level && skillLevel < ListGameObjects[i].level + 150)) {
								if (ListGameObjects[i].position.DistanceTo(localPlayer->position) < 5.0f) {
									if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
										ThreadSynchronizer::pressKey(0x28);
										ThreadSynchronizer::releaseKey(0x28);
										Moving = 0;
									}
									ThreadSynchronizer::RunOnMainThread([=]() {
										Functions::InteractObject(ListGameObjects[i].Pointer, 1);
									});
									gatheringCD = time(0);
									if (ListGameObjects[i].gatherType == 1) gatheringCD -= (time_t)1.5f;
								}
								else {
									ThreadSynchronizer::RunOnMainThread([=]() {
										Functions::MoveTo(ListGameObjects[i].position, 4);
									});
								}
								looted = true;
								break;
							}
						}
					}
					std::vector<bool> already_looted;
					for (int y = 0; y <= NumGroupMembers; y++) {
						already_looted.push_back(false);
					}
					for (unsigned int i = 0; i < ListUnits.size(); i++) {
						if (looted) break;
						bool lootable = (ListUnits[i].dynamic_flags & DYNAMICFLAG_CANBELOOTED);
						bool skinnable = (ListUnits[i].flags & UNIT_FLAG_SKINNABLE);
						float min_dist = localPlayer->position.DistanceTo(ListUnits[i].position);
						if (min_dist > 40.0f) continue;
						if (lootable) {
							bool skip = false;
							for (unsigned int y = 0; y < LootHistory.size(); y++) {
								if (get<0>(LootHistory[y]) == ListUnits[i].Guid) {
									skip = true;
									break;
								}
							}
							if (skip) continue;
							int player_close = 0;
							for (int y = 1; y <= NumGroupMembers; y++) {
								if (GroupMember[y] == NULL || (already_looted[y] == true) || (Leader != NULL && Leader->Guid == GroupMember[y]->Guid && !MCAutoMove)) continue;
								float dist = GroupMember[y]->position.DistanceTo(ListUnits[i].position);
								if (dist < min_dist) {
									min_dist = dist;
									player_close = y;
								}
							}
							already_looted[player_close] = true;
							if (player_close != 0) continue;
							else if (localPlayer->speed == 0.0f && ListUnits[i].position.DistanceTo(localPlayer->position) < 4.0f) {
								ThreadSynchronizer::RunOnMainThread([=]() {
									Functions::LootUnit(ListUnits[i].Pointer, 1);
								});
								LootHistory.push_back(std::tuple<unsigned long long, time_t>(ListUnits[i].Guid, time(0)));
								looted = true;
								break;
							}
							else if (!Functions::enemyClose(ListUnits[i].position, 20.0f)) {
								ThreadSynchronizer::RunOnMainThread([=]() {
									Functions::MoveTo(ListUnits[i].position, 4);
								});
								looted = true;
								break;
							}
						}
						else if (skinnable && skinningLevel > 0 && (ListUnits[i].level <= 20 && ((ListUnits[i].level-10)*10 <= skinningLevel && !Functions::enemyClose(ListUnits[i].position, 20.0f))
							|| (ListUnits[i].level > 20 && (ListUnits[i].level*5 <= skinningLevel)))) {
							if (localPlayer->speed == 0.0f && ListUnits[i].position.DistanceTo(localPlayer->position) < 4.0f) {
								ThreadSynchronizer::RunOnMainThread([=]() {
									Functions::LootUnit(ListUnits[i].Pointer, 1);
								});
								LootHistory.push_back(std::tuple<unsigned long long, time_t>(ListUnits[i].Guid, time(0)));
								looted = true;
								break;
							}
							else if (!Functions::enemyClose(ListUnits[i].position, 20.0f)) {
								ThreadSynchronizer::RunOnMainThread([=]() {
									Functions::MoveTo(ListUnits[i].position, 4);
								});
								looted = true;
								break;
							}
						}
					}
					if (!looted && localPlayer->speed == 0 && !Combat) {
						//Trade
						ThreadSynchronizer::RunOnMainThread([=]() {
							bool traded = false;
							for (int y = 1; y <= NumGroupMembers; y++) {
								for (unsigned int i = 0; i < leaderInfos.size(); i++) {
									if (GroupMember[y] != NULL && GroupMember[y]->position.DistanceTo(localPlayer->position) < 10.0f && GroupMember[y]->name == get<0>(leaderInfos[i])) {
										if (!traded && (get<2>(leaderInfos[i]) == 4 || get<3>(leaderInfos[i]) == 4)) {
											//Tailoring
											int listID[] = { 2589, 2592, 3182, 4306, 4337, 4338, 10285, 14047, 14227, 14256 };
											for (const auto& item : virtualInventory) {
												for (unsigned int z = 0; z < 10; z++) {
													if (get<2>(item) == listID[z]) {
														FunctionsLua::PickupItem(get<0>(item), get<1>(item));
														FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
														traded = true;
													}
												}
											}
										}
										if (!traded && (get<2>(leaderInfos[i]) == 5 || get<3>(leaderInfos[i]) == 5)) {
											//Leatherworking
											int listID[] = { 783, 2318, 2319, 2934, 4232, 4234, 4235, 4304, 7392, 7428, 8154, 8165, 8167, 8368, 8169 };
											for (const auto& item : virtualInventory) {
												for (unsigned int z = 0; z < 15; z++) {
													if (get<2>(item) == listID[z]) {
														FunctionsLua::PickupItem(get<0>(item), get<1>(item));
														FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
														traded = true;
													}
												}
											}
										}
										if (!traded && (get<2>(leaderInfos[i]) == 6 || get<3>(leaderInfos[i]) == 6)) {
											//Blacksmithing
											int listID[] = { 2770, 2771, 2772, 2775, 2776, 2835, 2836, 2838, 3858, 7912, 10620, 11370 };
											for (const auto& item : virtualInventory) {
												for (unsigned int z = 0; z < 12; z++) {
													if (get<2>(item) == listID[z]) {
														FunctionsLua::PickupItem(get<0>(item), get<1>(item));
														FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
														traded = true;
													}
												}
											}
										}
										if (!traded && (get<2>(leaderInfos[i]) == 8 || get<3>(leaderInfos[i]) == 8)) {
											//Alchemy
											int listID[] = { 765, 785, 2447, 2449, 2450, 2452, 2453, 3355, 3356, 3357, 3358, 3369, 3818, 3819, 3820, 3821, 4625, 8831, 8836, 8838, 8839, 8845 };
											for (const auto& item : virtualInventory) {
												for (unsigned int z = 0; z < 22; z++) {
													if (get<2>(item) == listID[z]) {
														FunctionsLua::PickupItem(get<0>(item), get<1>(item));
														FunctionsLua::DropItemOnUnit(tarType + std::to_string(y));
														traded = true;
													}
												}
											}
										}
										if (traded) break;
									}
								}
								if (traded) break;
							}
						});
					}
					if (!looted && Moving != 8 && Leader != NULL && Leader->Guid != localPlayer->Guid && (Leader->position.DistanceTo(localPlayer->position) > 5.0f)) {
						//Follow
						Functions::FollowMultibox(positionCircle);
						Moving = 4;
					}
				}
				if (localPlayer->speed > 0 && Moving > 0 && (Leader == NULL || Leader->Guid != localPlayer->Guid || MCAutoMove) && (playerLastPos.DistanceTo(localPlayer->position) < 0.5f)) {
					//Jump
					ThreadSynchronizer::pressKey(0x20); //Jump
					ThreadSynchronizer::releaseKey(0x20);
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
				if (localPlayer->className == "Druid") {
					if (playerSpec == 0) ListAI::DruidBalance();
					else if (playerSpec == 2) ListAI::DruidFeralCat();
					else if (playerSpec == 3) ListAI::DruidHeal();
				}
				else if (localPlayer->className == "Hunter") ListAI::HunterDps();
				else if (localPlayer->className == "Mage") ListAI::MageDps();
				else if (localPlayer->className == "Paladin") {
					if (playerSpec == 0) ListAI::PaladinHeal();
					else if (playerSpec == 1) ListAI::PaladinTank();
					else if (playerSpec == 2) ListAI::PaladinDps();
				}
				else if (localPlayer->className == "Priest") ListAI::PriestHeal();
				else if (localPlayer->className == "Rogue") ListAI::RogueDps();
				else if (localPlayer->className == "Warlock") ListAI::WarlockDps();
				else if (localPlayer->className == "Warrior" && playerSpec == 2) ListAI::WarriorTank();
			}

			if (localPlayer != NULL) playerLastPos = localPlayer->position;

			/*end = std::chrono::high_resolution_clock::now();
			float_ms = end - start;
			std::cout << "Actions: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
			Sleep(333);
		}
		if (Moving > 0) {
			if (Moving < 3) ThreadSynchronizer::pressKey(0x28);
			ThreadSynchronizer::releaseKey(0x28);
			Moving = 0;
		}
		Sleep(333);
	}
	Navigation::DisconnectClient();
}

std::vector<WoWUnit*> HasAggro[40]; std::vector<std::tuple<unsigned long long, time_t>> LootHistory;
bool Combat = false, IsSitting = false, IsFacing = false, hasTargetAggro = false, MCNoAuto = false, MCAutoMove = false,
los_target = false, passiveGroup = false, autoLearnSpells = false;
int AoEHeal = 0, nbrEnemy = 0, nbrCloseEnemy = 0, nbrCloseEnemyFacing = 0, nbrEnemyPlayer = 0, Moving = 0, NumGroupMembers = 0, playerSpec = 0, positionCircle = 0,
skinningLevel = 0, miningLevel = 0, herbalismLevel = 0, mapID = -1, keybindTrigger = 0, IsInGroup = 0;
unsigned int LastTarget = 0;
float distTarget = 0;
std::string tarType = "party", srcPath="";
std::vector<std::tuple<std::string, int, int, int>> leaderInfos;
std::vector<std::tuple<int, int, int, std::string>> virtualInventory;
std::vector<int> HealTargetArray;
WoWUnit* ccTarget = NULL; WoWUnit* targetUnit = NULL; WoWUnit* TankTarget = NULL; WoWUnit* GroupMember[40]; WoWUnit* PartyMember[5]; WoWUnit* Leader = NULL; WoWUnit* PvPTarget = NULL;
time_t current_time = time(0), autoAttackCD = time(0), gatheringCD = time(0);
Position playerLastPos = Position(0, 0, 0);
