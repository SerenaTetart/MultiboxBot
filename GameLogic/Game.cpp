#include "Game.h"
#include "./Functions/FunctionsLua.h"
#include "Client.h"
#include "MemoryManager.h"
#include "ListAI.h"
#include "./BossAI/BossAI.h"
#include "Navigation.h"
#include "FactionTemplate.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <mutex>

static time_t jumpCD = time(0);

static unsigned long long playerGuid = 0;
static unsigned long long pastPlayerGuid = 0;
static bool TradeShow = false, TradePendingSelf = false;

void Game::MainLoop() {
	while (Client::client_running == true) {
		if (playerGuid == 0 || playerGuid != pastPlayerGuid) {
			ThreadSynchronizer::RunOnMainThread(
				[]() {
					playerGuid = Functions::GetPlayerGuid();
					if (playerGuid > 0 && playerGuid != pastPlayerGuid) {
						Functions::EnumerateVisibleObjects(0);

						if (localPlayer != NULL) {
							pastPlayerGuid = playerGuid;
							std::string msg =
								"Name " + FunctionsLua::UnitName("player") +
								" Class " + localPlayer->className +
								" Faction " + (FactionTemplate.IsHorde(localPlayer->factionTemplateID) ? "1" : "0");
							Client::sendMessage(msg);

							std::string listSkills[] = { "Skinning", "Mining", "Herbalism", "Tailoring", "Leatherworking", "Blacksmithing", "Enchanting", "Alchemy", "Engineering" };
							int skills[] = { 0, 0 };
							std::tie(skills[0], skills[1]) = FunctionsLua::GetTradeSkillList(listSkills, std::size(listSkills));
							msg = ("Craft" + std::to_string(skills[0]) + std::to_string(skills[1]));
							Client::sendMessage(msg);
						}
					}
				}
			);
		}

		while (Client::bot_running == true && ((Leader != NULL && (Leader->Guid != playerGuid)) || !MCNoAuto)) {
			// ========================================== //
			// ===========   Initialisation   =========== //
			// ========================================== //
			/*auto start = std::chrono::high_resolution_clock::now();
			std::cout << "Initialisation start\n";*/

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
                        if (localPlayer != nullptr) BossAI::UpdateDangerZones();

						if (playerGuid != pastPlayerGuid && localPlayer != NULL) {
							pastPlayerGuid = playerGuid;
							std::string msg =
								"Name " + FunctionsLua::UnitName("player") +
								" Class " + localPlayer->className +
								" Faction " + (FactionTemplate.IsHorde(localPlayer->factionTemplateID) ? "1" : "0");
							Client::sendMessage(msg);

							std::string listSkills[] = { "Skinning", "Mining", "Herbalism", "Tailoring", "Leatherworking", "Blacksmithing", "Enchanting", "Alchemy", "Engineering" };
							int skills[] = { 0, 0 };
							std::tie(skills[0], skills[1]) = FunctionsLua::GetTradeSkillList(listSkills, std::size(listSkills));
							msg = ("Craft" + std::to_string(skills[0]) + std::to_string(skills[1]));
							Client::sendMessage(msg);
						}

						if (localPlayer != NULL) {
							targetUnit = localPlayer->getTarget();

							FunctionsLua::MakeVirtualInventory(&virtualInventory);
							Functions::MakeVirtualSpellBook(&virtualSpellBook);

							FunctionsLua::UpdateTradeSkills();

							if (FunctionsLua::GetRepairAllCost() > 0) Functions::LuaCall("RepairAllItems()");
							if ((localPlayer->channelInfo == 0) && (localPlayer->castInfo == 0) && FunctionsLua::GetMerchantNumItems() > 0) FunctionsLua::SellUselessItems();

							if (keybindTrigger == 1) {
								FunctionsLua::UseItem(6948);
								keybindTrigger = 0;
							}
							else if (keybindTrigger == 2) {
								UseMount();
								keybindTrigger = 0;
							}

							if (FunctionsLua::SpellIsTargeting()) FunctionsLua::SpellStopTargeting();

							inInstance = FunctionsLua::IsInInstance();
							const char* autoAttackTimer_char = (const char*)Functions::GetText("AATimer");
							autoAttackTimer = GetFloatFromChar(autoAttackTimer_char);
							if (std::strcmp(autoAttackTimer_char, "") == 0) {
								Functions::LuaCall(R"(
									AATimer = 0
									Multibox_BreathTimer = -1
									eventFrame = CreateFrame("Frame")
									eventFrame:RegisterEvent("TRADE_SHOW")
									eventFrame:RegisterEvent("TRADE_CLOSED")
									eventFrame:RegisterEvent("TRADE_ACCEPT_UPDATE")
									eventFrame:RegisterEvent("CHAT_MSG_MONSTER_YELL")
									eventFrame:RegisterEvent("CHAT_MSG_COMBAT_SELF_HITS")
									eventFrame:RegisterEvent("CHAT_MSG_COMBAT_SELF_MISSES")
									eventFrame:RegisterEvent("MIRROR_TIMER_START")
									eventFrame:RegisterEvent("MIRROR_TIMER_STOP")
									eventFrame:SetScript("OnEvent", function()
										if event == "TRADE_SHOW" then
											Multibox_TradeShow = 1
										elseif event == "TRADE_CLOSED" then
											Multibox_TradeShow = 0
											Multibox_TradePending = 0
											Multibox_TradePending2 = 0
										elseif event == "TRADE_ACCEPT_UPDATE" then
											if arg2 == 1 then Multibox_TradePending = 1
											elseif arg2 == 0 then Multibox_TradePending = 0 end
											if arg1 == 1 then Multibox_TradePending2 = 1
											elseif arg1 == 0 then Multibox_TradePending2 = 0 end
										elseif event == "CHAT_MSG_MONSTER_YELL" then
											last_yell_monster = arg1
										elseif(event == "CHAT_MSG_COMBAT_SELF_HITS" or event == "CHAT_MSG_COMBAT_SELF_MISSES") then
											AATimer = UnitAttackSpeed("player")
										elseif(event == "MIRROR_TIMER_START" and arg1 == "BREATH") then
											Multibox_BreathTimer = (arg2/1000)
										elseif(event == "MIRROR_TIMER_STOP" and arg1 == "BREATH") then
											Multibox_BreathTimer = -1
										end
									end)
									eventFrame:SetScript("OnUpdate", function()
										elapsed = (1/GetFramerate())
										AATimer = AATimer - elapsed
										if(AATimer < 0) then AATimer = 0 end
										Multibox_BreathTimer = Multibox_BreathTimer - elapsed
										if(Multibox_BreathTimer < 0) then Multibox_BreathTimer = 0 end
									end)
								)");
							}
							else {
								const char* tradeShowText = (const char*)Functions::GetText("Multibox_TradeShow");
								const char* tradePendingText = (const char*)Functions::GetText("Multibox_TradePending");
								const char* tradePendingText2 = (const char*)Functions::GetText("Multibox_TradePending2");
								TradeShow = tradeShowText && std::strcmp(tradeShowText, "1") == 0;
								bool TradePending = tradePendingText && std::strcmp(tradePendingText, "1") == 0;
								TradePendingSelf = tradePendingText2 && std::strcmp(tradePendingText2, "1") == 0;
								if (Combat && TradeShow) {
									Functions::LuaCall("CloseTrade()");
									TradeShow = false;
									TradePendingSelf = false;
								}
								else if (TradeShow && TradePending) Functions::LuaCall("AcceptTrade()");
								else if (TradeShow && !TradePending && !TradePendingSelf && FunctionsLua::GetTradePlayerItemLink(1) != "") {
									Functions::LuaCall("AcceptTrade()");
								}
								const char* breathTimer_char = (const char*)Functions::GetText("Multibox_BreathTimer");
								breathTimer = GetFloatFromChar(breathTimer_char);
							}

							if (localPlayer->isMounted && (Combat || ((Leader != NULL && !Leader->isMounted) && localPlayer->speed == 0 && !autoChores))) {
								Dismount();
							}
						}
					}
				}
			);

			if (localPlayer == NULL || playerGuid == 0) {
				Sleep(500);
				break;
			}

			Functions::ClassifyHeal();
			Functions::countEnemies(nbrEnemy, nbrCloseEnemy, nbrCloseEnemyFacing, nbrEnemyPlayer, ccTarget);

			Combat = (localPlayer->flags & UNIT_FLAG_IN_COMBAT) == UNIT_FLAG_IN_COMBAT;
            // Combat takes priority over travel, following and looting. Preserve tactical
            // movements (target approach, line of sight, boss mechanics and recovery).
            if (Combat && (Moving == MoveTypes::Journey || Moving == MoveTypes::Follow ||
                           Moving == MoveTypes::Looting)) {
                ThreadSynchronizer::RunOnMainThread([]() {
                    if (localPlayer && (Moving == MoveTypes::Journey || Moving == MoveTypes::Follow ||
                                        Moving == MoveTypes::Looting))
                        Functions::StopMovement();
                });
            }
            if (!Combat && Navigation::HasBlacklists()) Navigation::ClearBlacklists();
            bool lateralRecovering = false;
            ThreadSynchronizer::RunOnMainThread([&lateralRecovering]() {
                lateralRecovering = Functions::UpdateLateralRecovery();
            });
            if (Moving != MoveTypes::NotMoving && Navigation::HasCurrentMovement() &&
                (localPlayer->movement_flags & MOVEFLAG_ROOT)) {
                ThreadSynchronizer::RunOnMainThread([]() {
                    Functions::StopMovement();
                });
            }
            // Being stationary during a cast/channel is intentional, not a stuck path.
            if (!lateralRecovering && !(Combat && (localPlayer->castInfo || localPlayer->channelInfo)) &&
                Moving != MoveTypes::NotMoving && Navigation::HasCurrentMovement() &&
                !(localPlayer->movement_flags & (MOVEFLAG_ROOT | MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR)) &&
                Navigation::IsCurrentMovementStalled(localPlayer->position,
                    localPlayer->isMoving || (localPlayer->movement_flags &
                        (MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD | MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT)))) {
                ThreadSynchronizer::RunOnMainThread([]() {
                    Functions::HandleMovementStall();
                });
                ThreadSynchronizer::RunOnMainThread([&lateralRecovering]() {
                    lateralRecovering = Functions::UpdateLateralRecovery();
                });
            }

            if (Moving != MoveTypes::NotMoving &&
                Navigation::IsCurrentMovementUnsafe(static_cast<unsigned int>(mapID), localPlayer->position)) {
                ThreadSynchronizer::RunOnMainThread([]() {
                    Functions::StopMovement();
                });
            }

			IsFacing = false; distTarget = 0; hasTargetAggro = false;
			if (targetUnit != NULL) {
				if (targetUnit->attackable) IsFacing = localPlayer->isFacing(targetUnit->position, 0.3f);
				distTarget = localPlayer->position.DistanceTo(targetUnit->position) - localPlayer->combatReach - targetUnit->combatReach;
				for (unsigned int i = 0; i < HasAggro[0].size(); i++) {
					if (targetUnit->Guid == HasAggro[0][i]->Guid) {
						hasTargetAggro = true;
						break;
					}
				}
			}

			/*auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> float_ms = end - start;
			std::cout << "Initialisation: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
			// ========================================== //
			// ==============   Movements   ============= //
			// ========================================== //
			//start = std::chrono::high_resolution_clock::now();

			int RaptorStrikeIDs[8] = {2973, 14260, 14261, 14262, 14263, 14264, 14265, 14266};
			int HeroicStrikeIDs[9] = { 78, 284, 285, 1608, 11564, 11565, 11566, 11567, 25286 };
			int CleaveIDs[5] = { 845, 7369, 11608, 11609, 20569 };
            bool escapingDanger = false;
            if (Navigation::IsPositionDangerous(static_cast<unsigned int>(mapID), localPlayer->position)) {
                ThreadSynchronizer::RunOnMainThread([&escapingDanger]() {
                    escapingDanger = Functions::EscapeDangerZone();
                });
            }
            if (escapingDanger) { }
            else if (!lateralRecovering && breathTimer < 15.0f && breathTimer > 0.0f && (time(0) - jumpCD) >= 1.0f) {
				// Breathe quickly !
				ThreadSynchronizer::pressKey(0x20);
				ThreadSynchronizer::releaseKey(0x20);
				jumpCD = time(0);
			}
			else if (BossAI::BossAIAction()) { }
            else if (lateralRecovering) { }
			else if ((localPlayer->channelInfo == 0) && (localPlayer->castInfo == 0 || localPlayer->isCasting(RaptorStrikeIDs, 8) || localPlayer->isCasting(HeroicStrikeIDs, 9) || localPlayer->isCasting(CleaveIDs, 5))) {
				Position rangedLoSTarget;
				ThreadSynchronizer::RunOnMainThread([&rangedLoSTarget]() {
					if (Moving == MoveTypes::MoveLoSAoE && !groundAoERepositionRequested)
						Functions::StopMovement();
					groundAoERepositionRequested = false;
					los_target = true;
					if (targetUnit != NULL) {
						rangedLoSTarget = targetUnit->position;
						los_target = !Functions::Intersect(localPlayer->position, targetUnit->position);
						if (IsInGroup && (Leader != NULL) && (Leader->Guid != localPlayer->Guid) && targetUnit->attackable
							&& !(targetUnit->isInCombatOrEncounter()) && (targetUnit->Guid != Leader->targetGuid))
							Functions::LuaCall("ClearTarget()");
					}
				});
				//Reset Loot History after 20 sec
				unsigned int ind = 0;
				while (ind < LootHistory.size()) {
					if (float(time(0) - get<1>(LootHistory[ind])) >= 20.0f) LootHistory.erase(LootHistory.begin() + ind);
					else ind++;
				}
				if (localPlayer->isdead && localPlayer->health == 1) {
					CorpseRun();
				}
				else if (!Combat && autoChores) {
					// Go chores: train spells, train trading skills, sell/repair, etc...
					if (!lateralRecovering) DoChores();
				}
				else if (!passiveGroup && (Moving != MoveTypes::BossMechanic) && (Leader == NULL || (Leader->Guid != localPlayer->Guid) || MCAutoMove || Leader->indexGroup != 0) && (targetUnit != NULL) && targetUnit->attackable && !targetUnit->isdead) {
					if (localPlayer->movement_flags & MOVEFLAG_ROOT) {
						Moving = MoveTypes::NotMoving;
						// Nothing to do => face target
						if (!IsFacing && !localPlayer->isMoving &&
                            !(Combat && Functions::PlayerIsRanged())) {
							ThreadSynchronizer::RunOnMainThread([]() {
								if (localPlayer->isMounted) Dismount();
								localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position);
							});
						}
					}
					else if (Functions::PlayerIsRanged()) {
						float maxRange = 30.0f;
						if (localPlayer->className == "Hunter") maxRange = 35.0f;
						// !LoS => Find LoS
						if (!los_target &&
							(Moving == MoveTypes::NotMoving || Moving == MoveTypes::Follow ||
							 Moving == MoveTypes::GoToTarget || Moving == MoveTypes::MoveLoS ||
							 Moving == MoveTypes::MoveLoSAoE)) {
							const Position targetPosition = rangedLoSTarget;
							const bool targetSwimming = targetUnit->movement_flags & MOVEFLAG_SWIMMING;
							ThreadSynchronizer::RunOnMainThread([targetPosition, targetSwimming]() {
								Functions::MoveToLoS(targetPosition, MoveTypes::MoveLoS, targetSwimming);
							});
						}
						else if (Moving == MoveTypes::MoveLoSAoE && los_target) { }
						// Running and (target < 30 yard) => stop
						else if ((Moving == MoveTypes::Follow || Moving == MoveTypes::GoToTarget || Moving == MoveTypes::MoveLoSAlly) && distTarget < maxRange) {
                            ThreadSynchronizer::RunOnMainThread([]() {
                                Functions::StopMovement();
                            });
						}
						else if (distTarget > maxRange && !IsSitting) {
							// Target > 30 yard => Run to it
							bool targetSwim = false; if (targetUnit->movement_flags & MOVEFLAG_SWIMMING) targetSwim = true;
							ThreadSynchronizer::RunOnMainThread([=]() {
								Functions::MoveTo(targetUnit->position, MoveTypes::GoToTarget, true, targetSwim);
							});
						}
						else if (Moving == MoveTypes::MoveLoS && los_target) {
							// Looking for LoS, found it => stop
                            ThreadSynchronizer::RunOnMainThread([]() {
                                Functions::StopMovement();
                            });
						}
						else if (Moving == MoveTypes::RunAway && (distTarget > 15.0f || ((!(targetUnit->flags & UNIT_FLAG_STUNNED) && !(targetUnit->movement_flags & MOVEFLAG_ROOT))
							&& ((!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && hasTargetAggro) || ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed > 4.5))))) {
							// Walking backward and (target > 12 yard || Creature aggro || target running)
                            ThreadSynchronizer::RunOnMainThread([]() {
                                Functions::StopMovement();
                            });
						}
						else if ((Moving == MoveTypes::NotMoving || Moving == MoveTypes::RunAway) && (distTarget < 15.0f
							&& ((targetUnit->flags & UNIT_FLAG_STUNNED) || (targetUnit->movement_flags & MOVEFLAG_ROOT)
								|| (!(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && !hasTargetAggro)
								|| ((targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && targetUnit->speed <= 4.5 && targetUnit->speed > 0)))) {
							// Walk backward
							ThreadSynchronizer::RunOnMainThread([]() {
								if (Functions::StepBack(targetUnit, MoveTypes::RunAway) == false) {
									localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position);
								}
							});
						}
						else if (!Combat && (Moving == MoveTypes::NotMoving) && !IsFacing && !localPlayer->isMoving) {
							// Nothing to do => face target
							Moving = MoveTypes::NotMoving;
							ThreadSynchronizer::RunOnMainThread([]() {
								if (localPlayer->isMounted) Dismount();
								localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position);
							});
						}
						else if (localPlayer->isMounted) {
							ThreadSynchronizer::RunOnMainThread([]() {
								Dismount();
							});
						}
					}
					else {
						if (distTarget > 1.5f && !IsSitting) {
							// Target > 1.5 yard => Run to it
							bool targetSwim = false; if (targetUnit->movement_flags & MOVEFLAG_SWIMMING) targetSwim = true;
							ThreadSynchronizer::RunOnMainThread([=]() {
								if (Leader != NULL && (Leader->Guid == localPlayer->Guid) && Leader->indexGroup != 0) Functions::MoveTo(targetUnit->position, MoveTypes::GoToTarget, false, targetSwim);
								else Functions::MoveTo(targetUnit->position, MoveTypes::GoToTarget, true, targetSwim);
							});
						}
						else if (Moving == MoveTypes::NotMoving && !IsFacing && !localPlayer->isMoving) ThreadSynchronizer::RunOnMainThread([]() {
							Moving = MoveTypes::NotMoving;
							if (localPlayer->isMounted) Dismount();
							localPlayer->ClickToMove(FaceTarget, targetUnit->Guid, targetUnit->position);
						});
						else if (localPlayer->movement_flags & MOVEFLAG_FORWARD) {
							ThreadSynchronizer::pressKey(0x28);
							ThreadSynchronizer::releaseKey(0x28);
							Moving = MoveTypes::NotMoving;
						}
						else if (localPlayer->isMounted) {
							ThreadSynchronizer::RunOnMainThread([]() {
								Dismount();
							});
						}
						else Moving = MoveTypes::NotMoving;
					}
				}
				else if (Moving == MoveTypes::MoveLoSAlly && !localPlayer->isMoving && !los_target && (targetUnit != NULL) && (targetUnit->unitReaction >= Friendly)) {
					// Find LoS (ally)
					const Position targetPosition = targetUnit->position;
                    const bool targetSwimming = (targetUnit->movement_flags & MOVEFLAG_SWIMMING) != 0;
                    ThreadSynchronizer::RunOnMainThread([targetPosition, targetSwimming]() {
                        Functions::MoveToLoS(targetPosition, MoveTypes::MoveLoSAlly, targetSwimming);
                    });
				}
				else if (Moving == MoveTypes::MoveLoSAlly && (targetUnit == NULL || (targetUnit->unitReaction < Friendly) || ((targetUnit->unitReaction >= Friendly) && los_target))) {
					// Found LoS (ally)
                    ThreadSynchronizer::RunOnMainThread([]() {
                        Functions::StopMovement();
                    });
				}
				else if (
					(!Combat || passiveGroup) && !localPlayer->isdead && !IsSitting && (localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) &&
					(Leader == NULL || Leader->Guid != localPlayer->Guid || MCAutoMove || Leader->indexGroup != 0) &&
					(targetUnit == NULL || !targetUnit->attackable || targetUnit->isdead || passiveGroup)
				) {
					if( !Combat && Loot() ) { }
					else if (localPlayer->speed == 0 && !Combat && Trade()) { }
					else if (!localPlayer->isMounted && !localPlayer->isMoving && (localPlayer->movement_flags == MOVEFLAG_NONE) && Leader != NULL && Leader->isMounted && Game::CanUseMount()) {
						// Mount up !
						ThreadSynchronizer::RunOnMainThread([]() {
							UseMount();
						});
					}
					else if (Moving != MoveTypes::Journey && Leader != NULL && Leader->Guid != localPlayer->Guid && !(Leader->movement_flags & UNIT_FLAG_TAXI_FLIGHT) && (Leader->position.DistanceTo(localPlayer->position) > 5.0f || Moving == MoveTypes::Follow)) {
						// Follow
						Functions::FollowMultibox(positionCircle);
					}
				}
				else if ((localPlayer->movement_flags & MOVEFLAG_FORWARD) && Moving != MoveTypes::MoveLoSAlly && Moving != MoveTypes::Follow && Moving != MoveTypes::NotMoving) {
					ThreadSynchronizer::pressKey(0x28);
					ThreadSynchronizer::releaseKey(0x28);
					Moving = MoveTypes::NotMoving;
				}
				else if (localPlayer->movement_flags & MOVEFLAG_BACKWARD && Moving != MoveTypes::NotMoving) {
					ThreadSynchronizer::releaseKey(0x28);
					Moving = MoveTypes::NotMoving;
				}
				else if (!localPlayer->isMoving && Moving != MoveTypes::Follow && Moving != MoveTypes::NotMoving && Moving != MoveTypes::MoveLoSAlly) {
					Moving = MoveTypes::NotMoving;
				}
			}

            // Facing is independent of spellcasting: a channeled/cast spell may skip
            // the movement decision tree, but an idle ranged player still needs to turn.
            // Verify physical movement instead of trusting stale Moving/isMoving flags.
            constexpr unsigned int MovingFlags = MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD |
                MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT | MOVEFLAG_FALLING |
                MOVEFLAG_FALLINGFAR | MOVEFLAG_FLYING | MOVEFLAG_FLYING2;
            const bool stationary = !(localPlayer->movement_flags & MovingFlags) &&
                localPlayer->position.DistanceTo2D(playerLastPos) < 0.25f &&
                std::abs(localPlayer->position.Z - playerLastPos.Z) < 0.5f;
            if (Combat && !passiveGroup && !localPlayer->isdead &&
                Functions::PlayerIsRanged() && targetUnit && targetUnit->attackable &&
                !targetUnit->isdead && stationary &&
                (!Navigation::HasCurrentMovement() || localPlayer->castInfo || localPlayer->channelInfo) &&
                !localPlayer->isFacing(targetUnit->position, 0.3f)) {
                const unsigned long long facingTargetGuid = targetUnit->Guid;
                ThreadSynchronizer::RunOnMainThread([facingTargetGuid, movingFlags = MovingFlags]() {
                    if (!localPlayer || localPlayer->isdead ||
                        (localPlayer->movement_flags & movingFlags) ||
                        !(localPlayer->flags & UNIT_FLAG_IN_COMBAT) ||
                        localPlayer->targetGuid != facingTargetGuid) return;
                    WoWUnit* currentTarget = localPlayer->getTarget();
                    if (!currentTarget || currentTarget->Guid != facingTargetGuid ||
                        !currentTarget->attackable || currentTarget->isdead ||
                        localPlayer->isFacing(currentTarget->position, 0.3f)) return;
                    // An old waypoint cannot compete with a stationary cast and facing.
                    if (Navigation::HasCurrentMovement()) {
                        Navigation::ClearMovement();
                        Moving = MoveTypes::NotMoving;
                    }
                    localPlayer->ClickToMove(FaceTarget, facingTargetGuid, currentTarget->position);
                });
            }

			/*end = std::chrono::high_resolution_clock::now();
			float_ms = end - start;
			std::cout << "Movement: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
			// ========================================== //
			// ===============   Actions   ============== //
			// ========================================== //
			//start = std::chrono::high_resolution_clock::now();

			int drinkingIDs[15] = { 430, 431, 432, 1133, 1135, 1137, 24355, 25696, 26261, 26402, 26473, 26475, 29007, 10250, 22734 };
			if (IsSitting && (Combat || (localPlayer != NULL && (localPlayer->prctMana > 85)) || !localPlayer->hasBuff(drinkingIDs, 15))) {
				// Stop sitting
				IsSitting = false;
				ThreadSynchronizer::pressKey(0x28);
				ThreadSynchronizer::releaseKey(0x28);
				Moving = MoveTypes::NotMoving;
			}
			else if (localPlayer != NULL && !IsSitting && !localPlayer->isMounted) {
				if (!Combat && (localPlayer->channelInfo == 0) && (localPlayer->castInfo == 0) && !localPlayer->isMoving && (localPlayer->movement_flags == MOVEFLAG_NONE) && (localPlayer->prctMana < 33 && localPlayer->prctMana > 0) && (FunctionsLua::HasDrink() > 0)) {
					// Drink
					IsSitting = true;
					ThreadSynchronizer::RunOnMainThread([]() { FunctionsLua::UseItem(FunctionsLua::HasDrink()); });
				}
				else if (localPlayer->className == "Druid") {
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

            if (Moving == MoveTypes::NotMoving && Navigation::HasCurrentMovement())
                Navigation::ClearMovement();
			if (localPlayer != NULL) playerLastPos = localPlayer->position;

			/*end = std::chrono::high_resolution_clock::now();
			float_ms = end - start;
			std::cout << "Actions: elapsed time is " << float_ms.count() << " milliseconds" << std::endl;*/
			Sleep(500);
		}
        if (Moving != MoveTypes::NotMoving)
            ThreadSynchronizer::RunOnMainThread([]() { Functions::StopMovement(); });
		Sleep(500);
	}
	Navigation::DisconnectClient();
}

std::vector<WoWUnit*> HasAggro[40]; std::vector<std::tuple<unsigned long long, time_t>> LootHistory;
bool Combat = false, IsSitting = false, IsFacing = false, hasTargetAggro = false, MCNoAuto = false, MCAutoMove = false,
los_target = false, passiveGroup = false, inInstance = false, inventoryFull = false;
bool groundAoERepositionRequested = false;
int AoEHeal = 0, nbrEnemy = 0, nbrCloseEnemy = 0, nbrCloseEnemyFacing = 0, nbrEnemyPlayer = 0, NumGroupMembers = 0, playerSpec = 0, positionCircle = 0,
mapID = -1, keybindTrigger = 0, IsInGroup = 0, autoChores = 0;
unsigned long long LastTarget = 0;
float distTarget = 0, autoAttackTimer = 0, breathTimer = 0;
MoveTypes Moving;
std::string tarType = "party";
std::mutex leaderInfosMutex;
std::vector<std::tuple<std::string, int, int, int>> leaderInfos;
std::vector<InventoryItem> virtualInventory;
std::vector<SpellSlotData> virtualSpellBook;
std::vector<int> HealTargetArray;
WoWUnit* ccTarget = NULL; WoWUnit* targetUnit = NULL; WoWUnit* GroupMember[40]; WoWUnit* PartyMember[5]; WoWUnit* Leader = NULL;
time_t current_time = time(0);
Position playerLastPos = Position(0, 0, 0);

std::vector<std::tuple<std::string, int, int, int>> Game::GetLeaderInfosSnapshot() {
    std::lock_guard<std::mutex> lock(leaderInfosMutex);
    return leaderInfos;
}
