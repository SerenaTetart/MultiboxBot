#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static float SliceDiceDuration = 0; //Depends on talent and combo points

void ListAI::RogueDps() {
	float SliceDiceTimer = SliceDiceDuration - (time(0) - current_time);
	if (SliceDiceTimer < 0) SliceDiceTimer = 0;
	if (localPlayer->castInfo == 0 && localPlayer->channelInfo == 0 && !localPlayer->isdead && !passiveGroup) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			int nbrAggro = HasAggro[0].size();
			bool IsStunned = localPlayer->flags & UNIT_FLAG_STUNNED;
			bool IsConfused = localPlayer->flags & UNIT_FLAG_CONFUSED;
			int StealthIDs[4] = { 1784, 1785, 1786, 1787 };
			bool StealthBuff = localPlayer->hasBuff(StealthIDs, 4);
			int SliceDiceIDs[2] = { 5171, 6774 };
			bool SliceDiceBuff = localPlayer->hasBuff(SliceDiceIDs, 2);

			int ComboPoints = FunctionsLua::GetComboPoints();
			int SprintTalent = FunctionsLua::GetTalentInfo(2, 9);

			ListAI::DPSTargeting();

			if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
				//Healthstone
				FunctionsLua::UseHealthstone();
			}
			else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
				//Healing Potion
				FunctionsLua::UseHPotion();
			}
			else if (targetUnit != NULL && targetUnit->attackable) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
				bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
				int GougeIDs[5] = { 1776, 1777, 8629, 11285, 11286 };
				bool GougeDebuff = targetUnit->hasDebuff(GougeIDs, 5);
				bool stopAttack = false;
				if ((StealthBuff || GougeDebuff || targetConfused) && FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
				if (GougeDebuff || targetConfused) stopAttack = true; else stopAttack = false;
				if (!Combat && !StealthBuff && FunctionsLua::IsSpellReady("Stealth")) {
					//Stealth
					FunctionsLua::CastSpellByName("Stealth");
				}
				else if (StealthBuff && !targetStunned && FunctionsLua::IsSpellReady("Cheap Shot")) {
					//Cheap Shot
					FunctionsLua::CastSpellByName("Cheap Shot");
				}
				else if (stopAttack) { } //Do nothing
				else if (Combat && (distTarget > 12) && (localPlayer->speed < 7) && (SprintTalent > 0) && FunctionsLua::IsSpellReady("Sprint")) {
					//Sprint
					FunctionsLua::CastSpellByName("Sprint");
				}
				else if (Combat && !StealthBuff && (FunctionsLua::GetItemCount(5140) > 0) && (localPlayer->speed < 7) && FunctionsLua::IsSpellReady("Vanish")) {
					//Vanish
					FunctionsLua::CastSpellByName("Vanish");
				}
				else if (Combat && !StealthBuff && targetPlayer && (distTarget > 5) && (FunctionsLua::GetItemCount(5530) > 0) && (localPlayer->speed < 7) && FunctionsLua::IsSpellReady("Blind")) {
					//Blind
					FunctionsLua::CastSpellByName("Blind");
				}
				else if (Combat && !StealthBuff && SliceDiceBuff && FunctionsLua::UnitIsElite("target") && FunctionsLua::IsSpellReady("Adrenaline Rush")) {
					//Adrenaline Rush
					FunctionsLua::CastSpellByName("Adrenaline Rush");
				}
				else if (Combat && !StealthBuff && ((nbrCloseEnemyFacing >= 2) || targetPlayer) && (SliceDiceTimer >= 15) && FunctionsLua::IsSpellReady("Blade Flurry")) {
					//Blade Flurry
					FunctionsLua::CastSpellByName("Blade Flurry");
				}
				else if (!StealthBuff && (((ComboPoints >= 3) && !SliceDiceBuff) || (SliceDiceTimer < 8 && (ComboPoints >= 5))) && FunctionsLua::IsSpellReady("Slice and Dice")) {
					//Slice and Dice
					int SliceDiceTalent = FunctionsLua::GetTalentInfo(1, 6);
					SliceDiceDuration = (6 + (3 * ComboPoints)) * (1 + (0.15f * SliceDiceTalent));
					current_time = time(0);
					FunctionsLua::CastSpellByName("Slice and Dice");
				}
				else if (IsFacing && !targetStunned && !StealthBuff && (ComboPoints >= 3) && targetPlayer && FunctionsLua::IsSpellReady("Kidney Shot")) {
					//Kidney Shot
					FunctionsLua::CastSpellByName("Kidney Shot");
				}
				else if (IsFacing && !StealthBuff && (ComboPoints >= 5) && (SliceDiceTimer >= 8) && FunctionsLua::IsSpellReady("Eviscerate")) {
					//Eviscerate
					FunctionsLua::CastSpellByName("Eviscerate");
				}
				else if (IsFacing && !StealthBuff && !targetStunned && FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Kick")) {
					//Kick
					FunctionsLua::CastSpellByName("Kick");
				}
				else if (IsFacing && !StealthBuff && FunctionsLua::IsSpellReady("Sinister Strike")) {
					//Sinister Strike
					FunctionsLua::CastSpellByName("Sinister Strike");
				}
			}
		});
	}
}