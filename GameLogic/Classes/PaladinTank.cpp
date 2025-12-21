#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static void PaladinAttack(int index_paladin) {
	ListAI::TankTargeting();
	if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
		bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
		bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
		int SoRIDs[9] = { 20154, 21084, 20287, 20288, 20289, 20290, 20291, 20292, 20293 };
		bool SoRBuff = localPlayer->hasBuff(SoRIDs, 9);
		int SoWIDs[3] = { 20166, 20356, 20357 };
		bool SoWBuff = localPlayer->hasBuff(SoWIDs, 3);
		bool SealBuff = (SoRBuff || SoWBuff);
		int SoWDebuffIDs[3] = { 20186, 20354, 20355 };
		bool SoWDebuff = targetUnit->hasDebuff(SoWDebuffIDs, 3);
		if (!FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
		if ((localPlayer->prctMana < 33) && !SealBuff && FunctionsLua::IsSpellReady("Seal of Wisdom")) {
			//Seal of Wisdom
			FunctionsLua::CastSpellByName("Seal of Wisdom");
		}
		else if (!SealBuff && FunctionsLua::IsSpellReady("Seal of Righteousness")) {
			//Seal of Righteousness
			FunctionsLua::CastSpellByName("Seal of Righteousness");
		}
		else if ((Functions::getNbrCreatureType(20, Undead, Demon) >= 4) && FunctionsLua::IsSpellReady("Holy Wrath")) {
			//Holy Wrath
			FunctionsLua::CastSpellByName("Holy Wrath");
		}
		else if ((nbrCloseEnemy >= 2) && FunctionsLua::IsSpellReady("Consecration")) {
			//Consecration
			FunctionsLua::CastSpellByName("Consecration");
		}
		else if (SealBuff && (distTarget < 10) && ((localPlayer->prctMana > 20) || (SoWBuff && !SoWDebuff && FunctionsLua::UnitIsElite("target"))) && FunctionsLua::IsSpellReady("Judgement")) {
			//Judgement
			FunctionsLua::CastSpellByName("Judgement");
		}
		else if (!targetStunned && !targetConfused && (distTarget < 10) && FunctionsLua::IsSpellReady("Hammer of Justice")) {
			//Hammer of Justice
			FunctionsLua::CastSpellByName("Hammer of Justice");
		}
		else if (((localPlayer->prctMana > 33) || (nbrCloseEnemy <= 1)) && (distTarget < 30) && ((targetUnit->creatureType == Undead) || (targetUnit->creatureType == Demon)) && FunctionsLua::IsSpellReady("Exorcism")) {
			//Exorcism
			FunctionsLua::CastSpellByName("Exorcism");
		}
		else if ((distTarget < 30) && (targetUnit->prctHP < 20) && (localPlayer->prctMana > 33) && FunctionsLua::IsSpellReady("Hammer of Wrath")) {
			//Hammer of Wrath
			FunctionsLua::CastSpellByName("Hammer of Wrath");
		}
	}
}

static int HealGroup(unsigned int indexP) { //Heal Players and Npcs
	float HpRatio = ListUnits[indexP].prctHP;
	unsigned long long healGuid = ListUnits[indexP].Guid;
	bool isPlayer = (healGuid == localPlayer->Guid);
	int ForbearanceID[1] = { 25771 };
	bool ForbearanceDebuff = ListUnits[indexP].hasDebuff(ForbearanceID, 1);
	int BoSIDs[2] = { 6940, 20729 };
	bool BoSacrificeBuff = ListUnits[indexP].hasBuff(BoSIDs, 2);
	bool isParty = false;
	int nbrAggro = HasAggro[0].size();
	if (!isPlayer) {
		for (int i = 1; i <= NumGroupMembers; i++) {
			if ((GroupMember[i] != NULL) && GroupMember[i]->Guid == ListUnits[indexP].Guid) isParty = true;
		}
	}
	float distAlly = localPlayer->position.DistanceTo(ListUnits[indexP].position);
	if (Combat && isParty && (distAlly < 40.0f) && (HpRatio < 20) && FunctionsLua::IsSpellReady("Lay on Hands")) {
		//Lay on Hands
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Lay on Hands");
		LastTarget = indexP;
		if (Combat) Functions::LuaCall("TargetLastEnemy()");
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 25) && !ForbearanceDebuff && FunctionsLua::IsSpellReady("Divine Protection")) {
		//Divine Protection / Divine Shield
		FunctionsLua::CastSpellByName("Divine Protection"); FunctionsLua::CastSpellByName("Divine Shield");
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
		//Healthstone
		FunctionsLua::UseHealthstone();
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
		//Healing Potion
		FunctionsLua::UseHPotion();
		return 0;
	}
	else if (Combat && isParty && (distAlly < 30.0f) && (HpRatio < 33) && !ForbearanceDebuff && FunctionsLua::IsSpellReady("Blessing of Protection")) {
		//Blessing of Protection
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Protection");
		LastTarget = indexP;
		if (Combat) Functions::LuaCall("TargetLastEnemy()");
		return 0;
	}
	else if (Combat && isParty && (distAlly < 30.0f) && (HpRatio < 60) && !BoSacrificeBuff && FunctionsLua::IsSpellReady("Blessing of Sacrifice")) {
		//Blessing of Sacrifice
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Sacrifice");
		LastTarget = indexP;
		if (Combat) Functions::LuaCall("TargetLastEnemy()");
		return 0;
	}
	else if ((!Combat || nbrAggro == 0) && (distAlly < 40.0f) && (HpRatio < 50) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Flash of Light")) {
		//Flash of Light
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Flash of Light");
		LastTarget = indexP;
		if (Combat) Functions::LuaCall("TargetLastEnemy()");
		return 0;
	}
	else if ((!Combat || nbrAggro == 0) && (distAlly < 40.0f) && (HpRatio < 33) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Holy Light")) {
		//Holy Light
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Holy Light");
		LastTarget = indexP;
		if (Combat) Functions::LuaCall("TargetLastEnemy()");
		return 0;
	}
	return 1;
}

void ListAI::PaladinTank() {
	int FoLIDs[6] = { 19750, 19939, 19940, 19941, 19942, 19943 };
	int holyLightIDs[9] = { 635, 639, 647, 1026, 1042, 3472, 10328, 10329, 25292 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(holyLightIDs, 9) && (ListUnits[LastTarget].prctHP > 80))
		|| (localPlayer->isCasting(FoLIDs, 6) && (ListUnits[LastTarget].prctHP > 95)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			bool BoSanctuaryExist = FunctionsLua::IsPlayerSpell("Blessing of Sanctuary");
			WoWUnit* BoSalvationTarget = NULL; WoWUnit* BoSanctuaryTarget = NULL; bool BoSanctuaryBuff = false;
			if (BoSanctuaryExist) {
				int BoSalvationIDs[1] = { 1038 };
				BoSalvationTarget = Functions::GetMissingBuff(BoSalvationIDs, 1, 0, 1);
				int BoSanctuaryIDs[4] = { 20911, 20912, 20913, 20914 };
				BoSanctuaryBuff = localPlayer->hasBuff(BoSanctuaryIDs, 4);
				BoSanctuaryTarget = Functions::GetMissingBuff(BoSanctuaryIDs, 1, 0, 2);
			}

			bool BoKingsExist = FunctionsLua::IsPlayerSpell("Blessing of Kings");
			WoWUnit* BoKingsTarget = NULL; bool BoKingsBuff = false;
			if (BoKingsExist) {
				int BoKingsIDs[1] = { 20217 };
				BoKingsBuff = localPlayer->hasBuff(BoKingsIDs, 1);
				BoKingsTarget = Functions::GetMissingBuff(BoKingsIDs, 1);
			}

			bool BoWisdomBuff = false; WoWUnit* BoMightTarget = NULL; WoWUnit* BoWisdomTarget = NULL;
			if (!BoSanctuaryExist && !BoKingsExist) {
				int BoWisdomIDs[6] = { 19742, 19850, 19852, 19853, 19854, 25290 };
				BoWisdomBuff = localPlayer->hasBuff(BoWisdomIDs, 6);
				BoWisdomTarget = Functions::GetMissingBuff(BoWisdomIDs, 6, 1);
				int BoMightIDs[7] = { 19740, 19834, 19835, 19836, 19837, 19838, 25291 };
				BoMightTarget = Functions::GetMissingBuff(BoMightIDs, 7, 2);
			}
			
			unsigned int index_paladin = 0;
			for (int i = 1; i <= NumGroupMembers; i++) {
				if (GroupMember[i] != NULL && localPlayer->indexGroup > GroupMember[i]->indexGroup && FunctionsLua::UnitClass(tarType+std::to_string(i)) == "Paladin") {
					index_paladin = index_paladin + 1;
					break;
				}
			}
			
			int DevotionAuraIDs[7] = { 465, 10290, 643, 10291, 1032, 10292, 10293 };
			bool DevotionAuraBuff = localPlayer->hasBuff(DevotionAuraIDs, 7);
			int RetributionAuraIDs[5] = { 7294, 10298, 10299, 10300, 10301 };
			bool RetributionAuraBuff = localPlayer->hasBuff(RetributionAuraIDs, 5);
			int RighteousFuryIDs[1] = { 25780 };
			bool RighteousFuryBuff = localPlayer->hasBuff(RighteousFuryIDs, 1);

			WoWUnit* PurifyTarget = FunctionsLua::GetGroupDispel("Disease", "Poison");
			WoWUnit* CleanseTarget = FunctionsLua::GetGroupDispel("Disease", "Poison", "Magic");
			WoWUnit* deadPlayer = Functions::GetGroupDead(1);
			if (!DevotionAuraBuff && index_paladin == 0 && FunctionsLua::IsPlayerSpell("Devotion Aura")) {
				//Devotion Aura
				FunctionsLua::CastSpellByName("Devotion Aura");
			}
			else if (!RetributionAuraBuff && index_paladin == 1 && FunctionsLua::IsPlayerSpell("Retribution Aura")) {
			    //Retribution Aura
			    FunctionsLua::CastSpellByName("Retribution Aura");
			}
			else if (!RighteousFuryBuff && FunctionsLua::IsSpellReady("Righteous Fury")) {
				//Righteous Fury
				FunctionsLua::CastSpellByName("Righteous Fury");
			}
			else if (!Combat && !localPlayer->isMoving && (deadPlayer != NULL) && FunctionsLua::IsSpellReady("Redemption")) {
				//Redemption
				localPlayer->SetTarget(deadPlayer->Guid);
				FunctionsLua::CastSpellByName("Redemption");
			}
			else if (!BoKingsBuff && BoKingsExist && FunctionsLua::IsSpellReady("Blessing of Kings")) {
				//Blessing of Kings (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Blessing of Kings");
			}
			else if ((BoKingsTarget != NULL) && BoKingsExist && FunctionsLua::IsSpellReady("Blessing of Kings")) {
				//Blessing of Kings (Group)
				localPlayer->SetTarget(BoKingsTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Kings");
			}
			else if (!BoSanctuaryBuff && BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Sanctuary")) {
				//Blessing of Sanctuary (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Blessing of Sanctuary");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if ((BoSanctuaryTarget != NULL) && BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Sanctuary")) {
				//Blessing of Sanctuary (Groupe)
				localPlayer->SetTarget(BoSalvationTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Sanctuary");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if ((BoSalvationTarget != NULL) && BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Salvation")) {
				//Blessing of Salvation (Groupe)
				localPlayer->SetTarget(BoSalvationTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Salvation");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if (!BoWisdomBuff && !BoKingsExist && !BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Wisdom")) {
				//Blessing of Wisdom (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Blessing of Wisdom");
			}
			else if ((BoWisdomTarget != NULL) && !BoKingsExist && !BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Wisdom")) {
				//Blessing of Wisdom (Group)
				localPlayer->SetTarget(BoWisdomTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Wisdom");
			}
			else if ((BoMightTarget != NULL) && !BoKingsExist && !BoSanctuaryExist && FunctionsLua::IsSpellReady("Blessing of Might")) {
				//Blessing of Might (Group)
				localPlayer->SetTarget(BoMightTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Might");
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease", "Poison") && FunctionsLua::IsSpellReady("Purify")) {
				//Purify (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Purify");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if ((PurifyTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Purify")) {
				//Purify (Groupe)
				localPlayer->SetTarget(PurifyTarget->Guid);
				FunctionsLua::CastSpellByName("Purify");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease", "Poison", "Magic") && FunctionsLua::IsSpellReady("Cleanse")) {
				//Cleanse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cleanse");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if ((CleanseTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Cleanse")) {
				//Cleanse (Groupe)
				localPlayer->SetTarget(CleanseTarget->Guid);
				FunctionsLua::CastSpellByName("Cleanse");
				if (Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else {
				int tmp = 1; unsigned int index = 0;
				while (tmp == 1 and index < HealTargetArray.size()) {
					tmp = HealGroup(HealTargetArray[index]);
					index = index + 1;
				}
				if (tmp == 1 && !passiveGroup) PaladinAttack(index_paladin);
			}
		});
	}
}
