#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

void ListAI::HunterDps() {
	int RaptorStrikeIDs[8] = { 2973, 14260, 14261, 14262, 14263, 14264, 14265, 14266 };
	if ((localPlayer->castInfo == 0 || localPlayer->isCasting(RaptorStrikeIDs, 8)) && ((localPlayer->channelInfo == 0) || (localPlayer->flags & UNIT_FLAG_FEIGN_DEATH)) && !localPlayer->isdead && !passiveGroup) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			int nbrAggro = HasAggro[0].size();
			bool IsStunned = localPlayer->flags & UNIT_FLAG_STUNNED;
			bool IsConfused = localPlayer->flags & UNIT_FLAG_CONFUSED;
			int TrueshotAuraIDs[3] = { 19506, 20905, 20906 };
			bool TrueshotAuraBuff = localPlayer->hasBuff(TrueshotAuraIDs, 3);
			int AspectMonkeyIDs[1] = { 13163 };
			bool AspectMonkeyBuff = localPlayer->hasBuff(AspectMonkeyIDs, 1);
			int AspectHawkIDs[7] = { 13165, 14318, 14319, 14320, 14321, 14322, 25296 };
			bool AspectHawkBuff = localPlayer->hasBuff(AspectHawkIDs, 7);

			//Specific for Volley cast:
			Position cluster_center = Position(0, 0, 0); int cluster_unit;
			std::tie(cluster_center, cluster_unit) = Functions::getAOETargetPos(25, 35);

			bool FeedingBuff = FunctionsLua::GetUnitBuff("pet", "Interface\\Icons\\Ability_Hunter_BeastTraining");

			ListAI::DPSTargeting();

			if (!FunctionsLua::HasPetUI() && FunctionsLua::IsSpellReady("Call Pet")) {
				//Call Pet
				FunctionsLua::CastSpellByName("Call Pet");
			}
			else if (FunctionsLua::UnitIsDeadOrGhost("pet") && FunctionsLua::IsSpellReady("Revive Pet")) {
				//Revive Pet
				FunctionsLua::CastSpellByName("Revive Pet");
			}
			else if (!Combat && !FeedingBuff && FunctionsLua::HasPetUI() && (FunctionsLua::GetPetHappiness() < 3) && !FunctionsLua::UnitIsDeadOrGhost("pet") && FunctionsLua::HasMeat()) {
				//Feed Pet
				FunctionsLua::CastSpellByName("Feed Pet");
				FunctionsLua::PlaceItem(120, FunctionsLua::HasMeat()); FunctionsLua::UseAction(120);
			}
			else if (!TrueshotAuraBuff && FunctionsLua::IsSpellReady("Trueshot Aura")) {
				//Trueshot Aura
				FunctionsLua::CastSpellByName("Trueshot Aura");
			}
			else if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
				//Healthstone
				FunctionsLua::UseHealthstone();
			}
			else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
				//Healing Potion
				FunctionsLua::UseHPotion();
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if ((nbrEnemyPlayer == 0) && (nbrAggro > 0) && IsInGroup && FunctionsLua::IsSpellReady("Feign Death")) {
				//Feign Death (Aggro PvE)
				FunctionsLua::CastSpellByName("Feign Death");
			}
			else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
				bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
				int WingClipIDs[3] = { 2974, 14267, 14268 };
				bool WingClipDebuff = targetUnit->hasDebuff(WingClipIDs, 3);
				int SerpentStingIDs[9] = { 1978, 13549, 13550, 13551, 13552, 13553, 13554, 13555, 25295 };
				bool SerpentStingDebuff = targetUnit->hasDebuff(SerpentStingIDs, 9);
				int HunterMarkIDs[4] = { 1130,  14323, 14324, 14325 };
				bool HunterMarkDebuff = targetUnit->hasDebuff(HunterMarkIDs, 4);
				int FreezingTrapIDs[3] = { 1499,  14310, 14311 };
				bool FreezingTrapDebuff = targetUnit->hasDebuff(FreezingTrapIDs, 3);
				bool attacking = FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"));
				bool autoShotInRange = FunctionsLua::IsActionInRange(FunctionsLua::GetSlot("Auto Shot"));
				if ((FreezingTrapDebuff || targetConfused) && attacking) FunctionsLua::CastSpellByName("Attack");
				else if (!autoShotInRange && !attacking) FunctionsLua::CastSpellByName("Attack");
				if (autoShotInRange && !FunctionsLua::IsAutoRepeatAction(FunctionsLua::GetSlot("Auto Shot"))) FunctionsLua::CastSpellByName("Auto Shot");
				if (targetUnit->flags & UNIT_FLAG_IN_COMBAT && FunctionsLua::HasPetUI()) {
					Functions::LuaCall("PetAttack()");
				}
				if ((distTarget < 5.0f) && (localPlayer->prctMana > 10) && targetPlayer && FunctionsLua::IsSpellReady("Feign Death")) {
					//Feign Death
					FunctionsLua::CastSpellByName("Feign Death");
				}
				else if (!Combat && (distTarget < 5.0f) && targetPlayer && FunctionsLua::IsSpellReady("Freezing Trap")) {
					//Freezing Trap
					FunctionsLua::CastSpellByName("Freezing Trap");
				}
				else if (!Combat && (nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Explosive Trap")) {
					//Explosive trap (AoE)
					FunctionsLua::CastSpellByName("Explosive Trap");
				}
				else if ((distTarget < 15.0f) && targetPlayer && !FreezingTrapDebuff && FunctionsLua::IsSpellReady("Scatter Shot")) {
					//Scatter Shot
					FunctionsLua::CastSpellByName("Scatter Shot");
				}
				else if ((distTarget < 5.0f) && targetPlayer && !WingClipDebuff && !FreezingTrapDebuff && FunctionsLua::IsSpellReady("Wing Clip")) {
					//Wing Clip
					FunctionsLua::CastSpellByName("Wing Clip");
				}
				else if (IsFacing && targetUnit->channelInfo > 0 && (distTarget < 15.0f) && FunctionsLua::IsSpellReady("Scatter Shot")) {
					//Scatter Shot (Silence)
					FunctionsLua::CastSpellByName("Scatter Shot");
				}
				else if ((distTarget < 5.0f) && (nbrAggro > 0) && !AspectMonkeyBuff && FunctionsLua::IsSpellReady("Aspect of the Monkey")) {
					//Aspect of the Monkey
					FunctionsLua::CastSpellByName("Aspect of the Monkey");
				}
				else if (((autoShotInRange && !targetPlayer) || ((distTarget > 20.0f) && targetPlayer)) && !AspectHawkBuff && FunctionsLua::IsSpellReady("Aspect of the Hawk")) {
					//Aspect of the Hawk
					FunctionsLua::CastSpellByName("Aspect of the Hawk");
				}
				else if (IsFacing && autoShotInRange && targetPlayer && FunctionsLua::IsSpellReady("Concussive Shot")) {
					//Concussive Shot (PvP)
					FunctionsLua::CastSpellByName("Concussive Shot");
				}
				else if (IsFacing && !targetPlayer && hasTargetAggro && (distTarget < 5.0f) && FunctionsLua::IsSpellReady("Disengage")) {
					//Disengage
					FunctionsLua::CastSpellByName("Disengage");
				}
				else if (IsFacing && (distTarget < 5.0f) && FunctionsLua::IsSpellReady("Mongoose Bite")) {
					//Mongoose Bite
					FunctionsLua::CastSpellByName("Mongoose Bite");
				}
				else if (IsFacing && (distTarget < 5.0f) && FunctionsLua::IsSpellReady("Raptor Strike")) {
					//Raptor Strike
					FunctionsLua::CastSpellByName("Raptor Strike");
				}
				else if (!localPlayer->isMoving && (cluster_unit >= 4) && FunctionsLua::IsSpellReady("Volley")) {
					//Volley
					FunctionsLua::CastSpellByName("Volley");
					Functions::ClickAOE(cluster_center);
				}
				else if (!HunterMarkDebuff && FunctionsLua::UnitIsElite("target") && FunctionsLua::IsSpellReady("Hunter's Mark")) {
					//Hunter's Mark
					FunctionsLua::CastSpellByName("Hunter's Mark");
				}
				else if (!localPlayer->isMoving && ((autoShotInRange && !targetPlayer) || ((distTarget > 20.0f) && targetPlayer)) && FunctionsLua::UnitIsElite("target") && FunctionsLua::IsSpellReady("Rapid Fire")) {
					//Rapid Fire
					FunctionsLua::CastSpellByName("Rapid Fire");
				}
				else if (IsFacing && autoShotInRange && targetPlayer && !SerpentStingDebuff && FunctionsLua::IsSpellReady("Serpent Sting")) {
					//Serpent Sting (PvP)
					FunctionsLua::CastSpellByName("Serpent Sting");
				}
				else if (IsFacing && autoShotInRange && (localPlayer->speed > 0) && FunctionsLua::IsSpellReady("Arcane Shot")) {
					//Arcane Shot (Movement)
					FunctionsLua::CastSpellByName("Arcane Shot");
				}
				else if (IsFacing && ((autoShotInRange && !targetPlayer) || ((distTarget > 20.0f) && targetPlayer)) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Aimed Shot")) {
					//Aimed Shot
					FunctionsLua::CastSpellByName("Aimed Shot");
				}
				else if (IsFacing && autoShotInRange && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Multi-Shot")) {
					//Multi-Shot
					FunctionsLua::CastSpellByName("Multi-Shot");
				}
			}
		});
	}
}