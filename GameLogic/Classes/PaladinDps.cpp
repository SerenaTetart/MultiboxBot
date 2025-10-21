#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static void PaladinAttack() {
	ListAI::DPSTargeting();
	if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
		bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
		bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
		int SotCIDs[6] = { 21082, 20162, 20305, 20306, 20307, 20308 };
		bool SotCBuff = localPlayer->hasBuff(SotCIDs, 6);
		int SotCDebuffIDs[6] = {21183, 20188, 20300, 20301, 20302, 20303};
		bool SotCDebuff = targetUnit->hasDebuff(SotCDebuffIDs, 6);
		int SoRIDs[9] = { 20154, 21084, 20287, 20288, 20289, 20290, 20291, 20292, 20293 };
		bool SoRBuff = localPlayer->hasBuff(SoRIDs, 9);
		int SoCIDs[5] = { 20375, 20915, 20918, 20919, 20920 };
		bool SoCBuff = localPlayer->hasBuff(SoCIDs, 5);
		int SoLIDs[1] = { 20165 };
		bool SoLBuff = localPlayer->hasBuff(SoLIDs, 1);
		//int SoLDebuffIDs[1] = { 20167 };
		//bool SoLDebuff = targetUnit->hasDebuff(SoLDebuffIDs, 1);
		bool SealBuff = (SoRBuff || SotCBuff || SoCBuff || SoLBuff);
		if (!FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
		if (!SealBuff && !SotCDebuff && FunctionsLua::UnitIsElite("target") && FunctionsLua::IsSpellReady("Seal of the Crusader")) {
			//Seal of the Crusader
			FunctionsLua::CastSpellByName("Seal of the Crusader");
		}
		else if (!SealBuff && FunctionsLua::IsSpellReady("Seal of Command")) {
			//Seal of Command
			FunctionsLua::CastSpellByName("Seal of Command");
		}
		else if (!SealBuff && !FunctionsLua::IsPlayerSpell("Seal of Command") && FunctionsLua::IsSpellReady("Seal of Righteousness")) {
			//Seal of Righteousness
			FunctionsLua::CastSpellByName("Seal of Righteousness");
		}
		else if ((localPlayer->prctMana > 50) && (nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Consecration")) {
			//Consecration
			FunctionsLua::CastSpellByName("Consecration");
		}
		else if (SealBuff && (distTarget < 10) && (localPlayer->prctMana > 33) && FunctionsLua::IsSpellReady("Judgement")) {
			//Judgement
			FunctionsLua::CastSpellByName("Judgement");
		}
		else if (!targetStunned && !targetConfused && (distTarget < 10) && FunctionsLua::IsSpellReady("Hammer of Justice")) {
			//Hammer of Justice
			FunctionsLua::CastSpellByName("Hammer of Justice");
		}
		else if ((Functions::getNbrCreatureType(20, Undead, Demon) >= 4) && FunctionsLua::IsSpellReady("Holy Wrath")) {
			//Holy Wrath
			FunctionsLua::CastSpellByName("Holy Wrath");
		}
		else if ((localPlayer->prctMana > 33) && (distTarget < 30) && ((targetUnit->creatureType == Undead) || (targetUnit->creatureType == Demon)) && FunctionsLua::IsSpellReady("Exorcism")) {
			//Exorcism
			FunctionsLua::CastSpellByName("Exorcism");
		}
		/*else if ((distTarget < 30) && (targetUnit->prctHP < 20) && (localPlayer->prctMana > 33) && FunctionsLua::IsSpellReady("Hammer of Wrath")) {
			//Hammer of Wrath
			FunctionsLua::CastSpellByName("Hammer of Wrath");
		}*/
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
	bool isTank = (Leader != NULL && ListUnits[indexP].Guid == Leader->Guid);
	bool DivineProtectionBuff = false;
	if (!isPlayer) {
		for (int i = 1; i <= NumGroupMembers; i++) {
			if ((GroupMember[i] != NULL) && GroupMember[i]->Guid == ListUnits[indexP].Guid) isParty = true;
		}
	}
	else {
		int DivineProtectionIDs[4] = { 498, 5573, 642, 1020 };
		DivineProtectionBuff = ListUnits[indexP].hasBuff(DivineProtectionIDs, 4);
	}
	float distAlly = localPlayer->position.DistanceTo(ListUnits[indexP].position);
	if (Combat && isParty && (distAlly < 40.0f) && (HpRatio < 20) && FunctionsLua::IsSpellReady("Lay on Hands")) {
		//Lay on Hands
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Lay on Hands");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 40) && !ForbearanceDebuff && FunctionsLua::IsSpellReady("Divine Protection")) {
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
	else if (Combat && (distAlly < 30.0f) && isParty && !isTank && (HpRatio < 33) && !ForbearanceDebuff && FunctionsLua::IsSpellReady("Blessing of Protection")) {
		//Blessing of Protection
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Protection");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (Combat && (distAlly < 30.0f) && isParty && (HpRatio < 60) && !BoSacrificeBuff && FunctionsLua::IsSpellReady("Blessing of Sacrifice")) {
		//Blessing of Sacrifice
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Sacrifice");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((HpRatio < 50) && (distAlly < 40.0f) && (localPlayer->prctMana > 33 || DivineProtectionBuff) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Holy Light")) {
		//Holy Light
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Holy Light");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((HpRatio < 85) && (distAlly < 40.0f) && localPlayer->prctMana > 33 && (DivineProtectionBuff || !Combat) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Flash of Light")) {
		//Flash of Light (in bubble only)
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Flash of Light");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = 5;
		return 0;
	}
	return 1;
}

void ListAI::PaladinDps() {
	int FoLIDs[6] = { 19750, 19939, 19940, 19941, 19942, 19943 };
	int holyLightIDs[9] = { 635, 639, 647, 1026, 1042, 3472, 10328, 10329, 25292 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(holyLightIDs, 9) && (ListUnits[LastTarget].prctHP > 80))
		|| (localPlayer->isCasting(FoLIDs, 6) && (ListUnits[LastTarget].prctHP > 95)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			int SanctityAuraIDs[1] = { 20218 };
			bool SanctityAuraBuff = localPlayer->hasBuff(SanctityAuraIDs, 1);
			int DevotionAuraIDs[7] = { 465, 10290, 643, 10291, 1032, 10292, 10293 };
			bool DevotionAuraBuff = localPlayer->hasBuff(DevotionAuraIDs, 7);
			
			bool BoSanctuaryExist = FunctionsLua::IsPlayerSpell("Blessing of Sanctuary");
			WoWUnit* BoSalvationTarget = NULL; WoWUnit* BoSanctuaryTarget = NULL;
			if (BoSanctuaryExist) {
				int BoSalvationIDs[1] = { 1038 };
				BoSalvationTarget = Functions::GetMissingBuff(BoSalvationIDs, 1, 0, 1);
				int BoSanctuaryIDs[4] = { 20911, 20912, 20913, 20914 };
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

			WoWUnit* PurifyTarget = FunctionsLua::GetGroupDispel("Disease", "Poison");
			WoWUnit* CleanseTarget = FunctionsLua::GetGroupDispel("Disease", "Poison", "Magic");
			WoWUnit* deadPlayer = Functions::GetGroupDead(1);
			if (!SanctityAuraBuff && FunctionsLua::IsPlayerSpell("Sanctity Aura")) {
				//Sanctity Aura
				FunctionsLua::CastSpellByName("Sanctity Aura");
			}
			else if (!DevotionAuraBuff && !FunctionsLua::IsPlayerSpell("Sanctity Aura") && FunctionsLua::IsPlayerSpell("Devotion Aura")) {
				//Devotion Aura
				FunctionsLua::CastSpellByName("Devotion Aura");
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
			}
			else if ((PurifyTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Purify")) {
				//Purify (Group)
				localPlayer->SetTarget(PurifyTarget->Guid);
				FunctionsLua::CastSpellByName("Purify");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease", "Poison", "Magic") && FunctionsLua::IsSpellReady("Cleanse")) {
				//Cleanse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cleanse");
			}
			else if ((CleanseTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Cleanse")) {
				//Cleanse (Group)
				localPlayer->SetTarget(CleanseTarget->Guid);
				FunctionsLua::CastSpellByName("Cleanse");
			}
			else {
				int tmp = 1; unsigned int index = 0;
				while (tmp == 1 and index < HealTargetArray.size()) {
					tmp = HealGroup(HealTargetArray[index]);
					index = index + 1;
				}
				if (tmp == 1 && !passiveGroup) PaladinAttack();
			}
		});
	}
}