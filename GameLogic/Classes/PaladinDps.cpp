#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static void PaladinAttack() {
	if (ListAI::DPSTargeting()) {}
	else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
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
		if (localPlayer->autoAttackGuid == 0) Functions::InteractUnit(targetUnit->Pointer, 1);
		if (!SealBuff && !SotCDebuff && targetUnit->isElite() && Functions::IsSpellReady("Seal of the Crusader")) {
			//Seal of the Crusader
			FunctionsLua::CastSpellByName("Seal of the Crusader");
		}
		else if (!SealBuff && Functions::IsSpellReady("Seal of Command")) {
			//Seal of Command
			FunctionsLua::CastSpellByName("Seal of Command");
		}
		else if (!SealBuff && !Functions::IsPlayerSpell("Seal of Command") && Functions::IsSpellReady("Seal of Righteousness")) {
			//Seal of Righteousness
			FunctionsLua::CastSpellByName("Seal of Righteousness");
		}
		else if (!localPlayer->isMoving && (nbrCloseEnemy >= 4) && Functions::IsSpellReady("Consecration")) {
			//Consecration
			FunctionsLua::CastSpellByName("Consecration");
		}
		else if (SealBuff && (distTarget < 10) && (localPlayer->prctMana > 33 || SotCBuff) && autoAttackTimer > 1.0f && ((SotCBuff && !SotCDebuff && targetUnit->isElite()) || !SotCBuff) && Functions::IsSpellReady("Judgement")) {
			//Judgement
			FunctionsLua::CastSpellByName("Judgement");
		}
		else if (!targetStunned && !targetConfused && (distTarget < 10) && Functions::IsSpellReady("Hammer of Justice")) {
			//Hammer of Justice
			FunctionsLua::CastSpellByName("Hammer of Justice");
		}
		else if ((Functions::getNbrCreatureType(20, Undead, Demon) >= 4) && Functions::IsSpellReady("Holy Wrath")) {
			//Holy Wrath
			FunctionsLua::CastSpellByName("Holy Wrath");
		}
		else if ((localPlayer->prctMana > 33) && (distTarget < 30) && ((targetUnit->creatureType == Undead) || (targetUnit->creatureType == Demon)) && Functions::IsSpellReady("Exorcism")) {
			//Exorcism
			FunctionsLua::CastSpellByName("Exorcism");
		}
		/*else if ((distTarget < 30) && (targetUnit->prctHP < 20) && (localPlayer->prctMana > 33) && Functions::IsSpellReady("Hammer of Wrath")) {
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
	if (Combat && (distAlly < 40.0f) && (HpRatio < 20) && Functions::IsSpellReady("Lay on Hands")) {
		//Lay on Hands
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Lay on Hands");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = MoveTypes::MoveLoSAlly;
		return 0;
	}
	else if (Combat && (distAlly < 30.0f) && isParty && !isTank && (HpRatio < 33) && !ForbearanceDebuff && Functions::IsSpellReady("Blessing of Protection")) {
		//Blessing of Protection
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Protection");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = MoveTypes::MoveLoSAlly;
		return 0;
	}
	else if (Combat && (distAlly < 30.0f) && isParty && (HpRatio < 40) && !BoSacrificeBuff && Functions::IsSpellReady("Blessing of Sacrifice")) {
		//Blessing of Sacrifice
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Blessing of Sacrifice");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = MoveTypes::MoveLoSAlly;
		return 0;
	}
	else if ((HpRatio < 50) && (distAlly < 40.0f) && (localPlayer->prctMana > 33 || DivineProtectionBuff) && !localPlayer->isMoving && Functions::IsSpellReady("Holy Light")) {
		//Holy Light
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Holy Light");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = MoveTypes::MoveLoSAlly;
		return 0;
	}
	else if ((HpRatio < 85) && (distAlly < 40.0f) && localPlayer->prctMana > 33 && (DivineProtectionBuff || !Combat || autoAttackTimer > 1.5f) && !localPlayer->isMoving && Functions::IsSpellReady("Flash of Light")) {
		//Flash of Light (in bubble or between attacks)
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Flash of Light");
		LastTarget = indexP;
		bool los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
		if (!los_heal) Moving = MoveTypes::MoveLoSAlly;
		return 0;
	}
	return 1;
}

enum class BlessingMode {
	WisdomMight,
	SalvationSanctuary,
	Kings,
	Light
};

void ListAI::PaladinDps() {
	int FoLIDs[6] = { 19750, 19939, 19940, 19941, 19942, 19943 };
	int holyLightIDs[9] = { 635, 639, 647, 1026, 1042, 3472, 10328, 10329, 25292 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(holyLightIDs, 9) && (ListUnits[LastTarget].prctHP > 80))
		|| (localPlayer->isCasting(FoLIDs, 6) && (ListUnits[LastTarget].prctHP > 95)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	ThreadSynchronizer::RunOnMainThread([]() {
		int ForbearanceID[1] = { 25771 };
		bool ForbearanceDebuff = localPlayer->hasDebuff(ForbearanceID, 1);
		if (Combat && (localPlayer->prctHP < 20) && !ForbearanceDebuff && Functions::IsSpellReady("Divine Protection")) {
			//Divine Protection
			FunctionsLua::CastSpellByName("Divine Protection");
		}
		else if (Combat && (localPlayer->prctHP < 20) && !ForbearanceDebuff && Functions::IsSpellReady("Divine Shield")) {
			//Divine Shield
			FunctionsLua::CastSpellByName("Divine Shield");
		}
		else if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
			//Healthstone
			FunctionsLua::UseHealthstone();
		}
		else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
			//Healing Potion
			FunctionsLua::UseHPotion();
		}
		else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
			BlessingMode blessingMode = BlessingMode::WisdomMight;
			WoWUnit* BoKingsTarget = NULL; bool BoKingsBuff = false;
			WoWUnit* BoMightTarget = NULL; WoWUnit* BoWisdomTarget = NULL; bool BoWisdomBuff = false;
			if (Functions::IsPlayerSpell("Blessing of Kings")) {
				blessingMode = BlessingMode::Kings;
				int BoKingsIDs[1] = { 20217 };
				BoKingsBuff = localPlayer->hasBuff(BoKingsIDs, 1);
				BoKingsTarget = Functions::GetMissingBuff(BoKingsIDs, 1);
			}
			else {
				int BoWisdomIDs[6] = { 19742, 19850, 19852, 19853, 19854, 25290 };
				BoWisdomBuff = localPlayer->hasBuff(BoWisdomIDs, 6);
				BoWisdomTarget = Functions::GetMissingBuff(BoWisdomIDs, 6, 1);
				int BoMightIDs[7] = { 19740, 19834, 19835, 19836, 19837, 19838, 25291 };
				BoMightTarget = Functions::GetMissingBuff(BoMightIDs, 7, 2);
			}

			int BoSIDs[2] = { 6940, 20729 };
			int DevotionAuraIDs[7] = { 465, 10290, 643, 10291, 1032, 10292, 10293 };
			bool DevotionAuraBuff = localPlayer->hasBuff(DevotionAuraIDs, 7);
			int RetributionAuraIDs[5] = { 7294, 10298, 10299, 10300, 10301 };
			bool RetributionAuraBuff = localPlayer->hasBuff(RetributionAuraIDs, 5);
			int SanctityAuraIDs[1] = { 20218 };
			bool SanctityAuraBuff = localPlayer->hasBuff(SanctityAuraIDs, 1);

			WoWUnit* PurifyTarget = FunctionsLua::GetGroupDispel("Disease", "Poison");
			WoWUnit* CleanseTarget = FunctionsLua::GetGroupDispel("Disease", "Poison", "Magic");
			WoWUnit* deadPlayer = Functions::GetGroupDead(1);
			if (!SanctityAuraBuff && Functions::IsPlayerSpell("Sanctity Aura")) {
				//Sanctity Aura
				FunctionsLua::CastSpellByName("Sanctity Aura");
			}
			else if (!DevotionAuraBuff && !Functions::IsPlayerSpell("Sanctity Aura") && !Functions::IsPlayerSpell("Retribution Aura") && Functions::IsPlayerSpell("Devotion Aura")) {
				//Devotion Aura
				FunctionsLua::CastSpellByName("Devotion Aura");
			}
			else if (!RetributionAuraBuff && !Functions::IsPlayerSpell("Sanctity Aura") && Functions::IsPlayerSpell("Retribution Aura")) {
				//Retribution Aura
				FunctionsLua::CastSpellByName("Retribution Aura");
			}
			else if (!Combat && !localPlayer->isMoving && (deadPlayer != NULL) && Functions::IsSpellReady("Redemption")) {
				//Redemption
				localPlayer->SetTarget(deadPlayer->Guid);
				FunctionsLua::CastSpellByName("Redemption");
			}
			else if (!BoKingsBuff && blessingMode == BlessingMode::Kings && !localPlayer->hasBuff(BoSIDs, 2) && (!Combat || autoAttackTimer > 1.5f) && Functions::IsSpellReady("Blessing of Kings")) {
				//Blessing of Kings (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Blessing of Kings");
			}
			else if ((BoKingsTarget != NULL) && blessingMode == BlessingMode::Kings && (!Combat || autoAttackTimer > 1.5f) && !BoKingsTarget->hasBuff(BoSIDs, 2) && Functions::IsSpellReady("Blessing of Kings")) {
				//Blessing of Kings (Group)
				localPlayer->SetTarget(BoKingsTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Kings");
			}
			else if (!BoWisdomBuff && blessingMode == BlessingMode::WisdomMight && !localPlayer->hasBuff(BoSIDs, 2) && (!Combat || autoAttackTimer > 1.5f) && Functions::IsSpellReady("Blessing of Wisdom")) {
				//Blessing of Wisdom (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Blessing of Wisdom");
			}
			else if ((BoWisdomTarget != NULL) && blessingMode == BlessingMode::WisdomMight && (!Combat || autoAttackTimer > 1.5f) && !BoWisdomTarget->hasBuff(BoSIDs, 2) && Functions::IsSpellReady("Blessing of Wisdom")) {
				//Blessing of Wisdom (Group)
				localPlayer->SetTarget(BoWisdomTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Wisdom");
			}
			else if ((BoMightTarget != NULL) && blessingMode == BlessingMode::WisdomMight && (!Combat || autoAttackTimer > 1.5f) && !BoMightTarget->hasBuff(BoSIDs, 2) && Functions::IsSpellReady("Blessing of Might")) {
				//Blessing of Might (Group)
				localPlayer->SetTarget(BoMightTarget->Guid);
				FunctionsLua::CastSpellByName("Blessing of Might");
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease", "Poison") && Functions::IsSpellReady("Purify")) {
				//Purify (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Purify");
			}
			else if ((PurifyTarget != NULL) && (localPlayer->prctMana > 25) && Functions::IsSpellReady("Purify")) {
				//Purify (Group)
				localPlayer->SetTarget(PurifyTarget->Guid);
				FunctionsLua::CastSpellByName("Purify");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease", "Poison", "Magic") && Functions::IsSpellReady("Cleanse")) {
				//Cleanse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cleanse");
			}
			else if ((CleanseTarget != NULL) && (localPlayer->prctMana > 25) && Functions::IsSpellReady("Cleanse")) {
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
		}
	});
}
