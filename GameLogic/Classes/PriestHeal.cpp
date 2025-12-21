#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

	//Local variables //
static int LesserHealRank = 0; static float LesserHealValue[3]; static int LesserHealLevel[3] = {1, 4, 10};
static int RenewRank = 0; static float RenewValue[10]; static int RenewLevel[10] = { 8, 14, 20, 26, 32, 38, 44, 50, 56, 60 };
static int HealRank = 0; static float HealValue[4]; static int HealLevel[4] = { 16, 22, 28, 34 };
static int GreaterHealRank = 0; static float GreaterHealValue[5]; static int GreaterHealLevel[5] = { 40, 46, 52, 58, 60 };
static int FlashHealRank = 0; static float FlashHealValue[7]; static int FlashHealLevel[7] = { 20, 26, 32, 38, 44, 50, 56 };
	//================//

static void GetSpellBonusHealing() {
	float tmp[3] = { 53, 84, 154 }; for (int i = 0; i < 3; i++) { LesserHealValue[i] = tmp[i]; }
	float tmp2[10] = { 45, 100, 175, 245, 315, 400, 510, 650, 810, 970 }; for (int i = 0; i < 10; i++) { RenewValue[i] = tmp2[i]; }
	float tmp3[4] = { 330, 476, 624, 781 }; for (int i = 0; i < 4; i++) { HealValue[i] = tmp3[i]; }
	float tmp4[5] = { 982, 1248, 1556, 1917, 2080 }; for (int i = 0; i < 5; i++) { GreaterHealValue[i] = tmp4[i]; }
	float tmp5[7] = { 224, 297, 372, 453, 583, 722, 885 }; for (int i = 0; i < 7; i++) { FlashHealValue[i] = tmp5[i]; }
	int RenewTalentRank = FunctionsLua::GetTalentInfo(2, 2);
	int SpiritualHealingRank = FunctionsLua::GetTalentInfo(2, 15);
	int spirit = FunctionsLua::UnitStat("player", 5);
	int SpiritualGuidance = FunctionsLua::GetTalentInfo(2, 14);
	float bonusHealing = spirit * 0.05f * SpiritualGuidance;
	std::tie(std::ignore, LesserHealRank) = FunctionsLua::GetSpellID("Lesser Heal");
	std::tie(std::ignore, RenewRank) = FunctionsLua::GetSpellID("Renew");
	std::tie(std::ignore, HealRank) = FunctionsLua::GetSpellID("Heal");
	std::tie(std::ignore, GreaterHealRank) = FunctionsLua::GetSpellID("Greater Heal");
	std::tie(std::ignore, FlashHealRank) = FunctionsLua::GetSpellID("Flash Heal");
	//====================================================//
	float SubLevel20PENALTY = 1.0f;
	if (RenewLevel[RenewRank] < 20.0f) SubLevel20PENALTY = 1.0f - (20.0f - RenewLevel[RenewRank]) * 0.0375f;
	RenewValue[RenewRank] = (RenewValue[RenewRank] + bonusHealing * SubLevel20PENALTY) * (1.0f + (0.05f * RenewTalentRank)) * (1.0f + (0.02f * SpiritualHealingRank));
	SubLevel20PENALTY = 1.0f - (20.0f - LesserHealLevel[LesserHealRank]) * 0.0375f;
	LesserHealValue[LesserHealRank] = (LesserHealValue[LesserHealRank] + bonusHealing * (2.5f / 3.5f) * SubLevel20PENALTY) * (1.0f + (0.02f * SpiritualHealingRank));
	SubLevel20PENALTY = 1.0f;
	if (HealLevel[HealRank] < 20.0f) SubLevel20PENALTY = 1.0f - (20.0f - HealLevel[HealRank]) * 0.0375f;
	HealValue[HealRank] = (HealValue[HealRank] + bonusHealing * (3.0f / 3.5f) * SubLevel20PENALTY) * (1.0f + (0.02f * SpiritualHealingRank));
	GreaterHealValue[GreaterHealRank] = (GreaterHealValue[GreaterHealRank] + bonusHealing * (3.0f / 3.5f)) * (1.0f + (0.02f * SpiritualHealingRank));
	FlashHealValue[FlashHealRank] = (FlashHealValue[FlashHealRank] + bonusHealing * (1.5f / 3.5f)) * (1.0f + (0.02f * SpiritualHealingRank));
}

static void PriestAttack() {
	ListAI::DPSTargeting();
	if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
		bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
		int ShadowWordPainIDs[8] = { 589, 594, 970, 992, 2767, 10892, 10893, 10894 };
		bool ShadowWordPainDebuff = targetUnit->hasDebuff(ShadowWordPainIDs, 8);
		int HolyFireIDs[8] = { 14914, 15262, 15263, 15264, 15265, 15266, 15267, 15261 };
		bool HolyFireDebuff = targetUnit->hasDebuff(HolyFireIDs, 8);
		if (!FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
		if ((nbrCloseEnemy >= 4) && localPlayer->prctMana > 40 && FunctionsLua::IsSpellReady("Holy Nova")) {
			//Holy Nova
			FunctionsLua::CastSpellByName("Holy Nova");
		}
		else if (IsFacing && !ShadowWordPainDebuff && targetUnit->getNbrDebuff() < 16 && !IsInGroup && FunctionsLua::IsSpellReady("Shadow Word: Pain")) {
			//Shadow Word: Pain
			FunctionsLua::CastSpellByName("Shadow Word: Pain");
		}
		else if (!Combat && IsFacing && !localPlayer->isMoving && !HolyFireDebuff && targetUnit->getNbrDebuff() < 16 && !IsInGroup && FunctionsLua::IsSpellReady("Holy Fire")) {
			//Holy Fire
			FunctionsLua::CastSpellByName("Holy Fire");
		}
		else if (IsFacing && !localPlayer->isMoving && !IsInGroup && FunctionsLua::IsSpellReady("Mind Blast")) {
			//Mind Blast
			FunctionsLua::CastSpellByName("Mind Blast");
		}
		else if (!Combat && IsFacing && !localPlayer->isMoving && !IsInGroup && FunctionsLua::IsSpellReady("Smite")) {
			//Smite
			FunctionsLua::CastSpellByName("Smite");
		}
		else if (IsFacing && !localPlayer->isMoving && FunctionsLua::HasWandEquipped() && !FunctionsLua::IsAutoRepeatAction(FunctionsLua::GetSlot("Shoot"))) {
			//Wand
			FunctionsLua::CastSpellByName("Shoot");
		}
	}
}

static int HealGroup(unsigned int indexP) { //Heal Players and Npcs
	unsigned long long healGuid = ListUnits[indexP].Guid;
	bool isPlayer = (healGuid == localPlayer->Guid);
	bool isParty = false;
	if (!isPlayer) {
		for (int i = 1; i <= NumGroupMembers; i++) {
			if ((GroupMember[i] != NULL) && GroupMember[i]->Guid == ListUnits[indexP].Guid) isParty = true;
		}
		if (!isParty) return 1;
	}
	bool los_heal = true; if(isParty) los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
	float HpRatio = ListUnits[indexP].prctHP;
	int HpLost = ListUnits[indexP].hpLost;
	int RenewIDs[10] = { 139, 6074, 6075, 6076, 6077, 6078, 10927, 10928, 10929, 25315 };
	bool RenewBuff = ListUnits[indexP].hasBuff(RenewIDs, 10);
	int PWShieldIDs[10] = { 17, 592, 600, 3747, 6065, 6066, 10898, 10899, 10900, 10901 };
	bool PWShieldBuff = ListUnits[indexP].hasBuff(PWShieldIDs, 10);
	int WeakenedSoulID[1] = { 6788 }; bool WeakenedSoulDebuff = ListUnits[indexP].hasDebuff(WeakenedSoulID, 1);
	float distAlly = localPlayer->position.DistanceTo(ListUnits[indexP].position);
	if (isPlayer && Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
		//Healthstone
		FunctionsLua::UseHealthstone();
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
		//Healing Potion
		FunctionsLua::UseHPotion();
		return 0;
	}
	else if (isPlayer && Combat && (localPlayer->prctHP < 35) && FunctionsLua::IsSpellReady("Desperate Prayer")) {
		//Desperate Prayer
		FunctionsLua::CastSpellByName("Desperate Prayer");
		return 0;
	}
	else if (Combat && (localPlayer->prctHP < 40) && (localPlayer->prctMana < 33) && FunctionsLua::IsSpellReady("Inner Focus")) {
		//Inner Focus
		FunctionsLua::CastSpellByName("Inner Focus");
		return 0;
	}
	else if ((AoEHeal >= 3) && (distAlly < 40.0f) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Prayer of Healing")) {
		//Prayer of Healing
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Prayer of Healing");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (((isPlayer && Combat && (HasAggro[0].size() > 0)) || (HpRatio < 35)) && !PWShieldBuff && !WeakenedSoulDebuff && (distAlly < 40.0f) && FunctionsLua::IsSpellReady("Power Word: Shield")) {
		//Power Word: Shield
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Power Word: Shield");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (Combat && (HpRatio < 30) && (distAlly < 40.0f) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Flash Heal")) {
		//Flash Heal
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Flash Heal");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((HpLost > GreaterHealValue[GreaterHealRank]) && (distAlly < 40.0f) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Greater Heal")) {
		//Greater Heal
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Greater Heal");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((HpLost > HealValue[HealRank]) && (distAlly < 40.0f) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Heal")) {
		//Heal
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Heal");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((localPlayer->level < 40) && (HpLost > LesserHealValue[LesserHealRank]) && (distAlly < 40.0f) && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Lesser Heal")) {
		//Lesser Heal
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Lesser Heal");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if ((HpLost > RenewValue[RenewRank]) && !RenewBuff && (distAlly < 40.0f) && FunctionsLua::IsSpellReady("Renew")) {
		//Renew
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Renew");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	return 1;
}

void ListAI::PriestHeal() {
	int LesserHealIDs[3] = { 2050, 2052, 2053 }; int FlashHealIDs[7] = { 2061, 9472, 9473, 9474, 10915, 10916, 10917 };
	int GreaterHealIDs[5] = { 2060, 10963, 10964, 10965, 25314 }; int HealIDs[4] = { 2054, 2055, 6093, 6064 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(LesserHealIDs, 3) && (ListUnits[LastTarget].hpLost < LesserHealValue[LesserHealRank] * 0.9))
		|| (localPlayer->isCasting(HealIDs, 4) && (ListUnits[LastTarget].hpLost < HealValue[HealRank] * 0.9))
		|| (localPlayer->isCasting(GreaterHealIDs, 5) && (ListUnits[LastTarget].hpLost < GreaterHealValue[GreaterHealRank] * 0.9))
		|| (localPlayer->isCasting(FlashHealIDs, 7) && (ListUnits[LastTarget].prctHP > 80)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			float SpellCalculTimer = 30.0f - (time(0) - current_time);
			if (SpellCalculTimer <= 0) {
				GetSpellBonusHealing();
				current_time = time(0);
			}
			int PWFortitudeIDs[8] = { 1243, 1244, 1245, 2791, 10937, 10938, 21562, 21564 }; //Include PoFortitude
			bool PWFortitudeBuff = localPlayer->hasBuff(PWFortitudeIDs, 8);
			WoWUnit* PWFortitudeTarget = Functions::GetMissingBuff(PWFortitudeIDs, 8);
			int DivineSpiritIDs[5] = { 14752, 14818, 14819, 27841, 27681 }; //Include PoSpirit
			bool DivineSpiritBuff = localPlayer->hasBuff(DivineSpiritIDs, 5);
			WoWUnit* DivineSpiritTarget = Functions::GetMissingBuff(DivineSpiritIDs, 5);
			int InnerFireIDs[6] = { 588, 7128, 602, 1006, 10951, 10952 };
			bool InnerFireBuff = localPlayer->hasBuff(InnerFireIDs, 6);
			WoWUnit* DispelMagicTarget = FunctionsLua::GetGroupDispel("Magic");
			WoWUnit* CureDiseaseTarget = FunctionsLua::GetGroupDispel("Disease");
			WoWUnit* deadPlayer = Functions::GetGroupDead();
			if (!Combat && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Resurrection") && (deadPlayer != NULL)) {
				//Resurrection
				localPlayer->SetTarget(deadPlayer->Guid);
				FunctionsLua::CastSpellByName("Resurrection");
			}
			else if (!Combat && (PWFortitudeTarget != NULL) && FunctionsLua::IsSpellReady("Prayer of Fortitude")) {
				//Prayer of Fortitude (Group)
				localPlayer->SetTarget(PWFortitudeTarget->Guid);
				FunctionsLua::CastSpellByName("Prayer of Fortitude");
			}
			else if (!Combat && (DivineSpiritTarget != NULL) && FunctionsLua::IsSpellReady("Prayer of Spirit")) {
				//Prayer of Spirit (Group)
				localPlayer->SetTarget(DivineSpiritTarget->Guid);
				FunctionsLua::CastSpellByName("Prayer of Spirit");
			}
			else if (!InnerFireBuff && FunctionsLua::IsPlayerSpell("Inner Fire")) {
				//Inner Fire (self)
				FunctionsLua::CastSpellByName("Inner Fire");
			}
			else if (!Combat && !PWFortitudeBuff && FunctionsLua::IsPlayerSpell("Power Word: Fortitude")) {
				//Power Word: Fortitude (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Power Word: Fortitude");
			}
			else if (!Combat && (PWFortitudeTarget != NULL) && FunctionsLua::IsSpellReady("Power Word: Fortitude")) {
				//Power Word: Fortitude (Group)
				localPlayer->SetTarget(PWFortitudeTarget->Guid);
				FunctionsLua::CastSpellByName("Power Word: Fortitude");
			}
			else if (!Combat && !DivineSpiritBuff && FunctionsLua::IsPlayerSpell("Divine Spirit")) {
				//Divine Spirit (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Divine Spirit");
			}
			else if (!Combat && (DivineSpiritTarget != NULL) && FunctionsLua::IsSpellReady("Divine Spirit")) {
				//Divine Spirit (Group)
				localPlayer->SetTarget(DivineSpiritTarget->Guid);
				FunctionsLua::CastSpellByName("Divine Spirit");
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if (IsInGroup && (HasAggro[0].size() > 0) && FunctionsLua::IsSpellReady("Fade")) {
				//Fade (Aggro)
				FunctionsLua::CastSpellByName("Fade");
			}
			else if ((nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Psychic Scream")) {
				//Psychic Scream
				FunctionsLua::CastSpellByName("Psychic Scream");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Disease") && FunctionsLua::IsSpellReady("Cure Disease")) {
				//Cure Disease (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cure Disease");
			}
			else if ((CureDiseaseTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Cure Disease")) {
				//Cure Disease (Group)
				localPlayer->SetTarget(CureDiseaseTarget->Guid);
				FunctionsLua::CastSpellByName("Cure Disease");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Magic") && FunctionsLua::IsSpellReady("Dispel Magic")) {
				//Dispel Magic (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Dispel Magic");
			}
			else if ((DispelMagicTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Dispel Magic")) {
				//Dispel Magic (Groupe)
				localPlayer->SetTarget(DispelMagicTarget->Guid);
				FunctionsLua::CastSpellByName("Dispel Magic");
			}
			else {
				//Priority in function of the number of heals in current group
				int tmp = 1; unsigned int index_start = 0;
				for (int i = 1; i <= NumGroupMembers; i++) {
					if (GroupMember[i] != NULL && GroupMember[i]->role == 3 && localPlayer->indexGroup > GroupMember[i]->indexGroup) {
						index_start = index_start + 1;
						break;
					}
				}
				unsigned int index = index_start; unsigned int n = HealTargetArray.size();
				do {
					tmp = HealGroup(HealTargetArray[index]);
					index = (index + 1)%n;
				} while (tmp == 1 && index != (index_start - 1)%n);
				if (tmp == 1 && !passiveGroup) PriestAttack();
			}
		});
	}
}
