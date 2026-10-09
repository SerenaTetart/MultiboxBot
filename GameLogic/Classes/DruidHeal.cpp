#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static time_t EntanglingRootsTimer = time(0);

static float RegrowthValue[9] = { 198, 364, 532, 700, 879, 1113, 1398, 1748, 2125 }; static int RegrowthLevel[9] = { 12, 18, 24, 30, 36, 42, 48, 54, 60 }; static float RegrowthComputed = 0;
static float HealingTouchValue[11] = { 48, 107, 229, 418, 651, 838, 1051, 1339, 1686, 2087, 2472 }; static int HealingTouchLevel[11] = { 1, 8, 14, 20, 26, 32, 38, 44, 50, 56, 60 }; static float HealingTouchComputed = 0;
static float RejuvenationValue[11] = { 32, 56, 116, 180, 244, 304, 388, 488, 608, 756, 888 }; static int RejuvenationLevel[11] = { 4, 10, 16, 22, 28, 34, 40, 46, 52, 58, 60 }; static float RejuvenationComputed = 0;

static void GetSpellBonusHealing() {
	int ImprovedRejuvenation = FunctionsLua::GetTalentInfo(3, 11);
	int GiftOfNatureRank = FunctionsLua::GetTalentInfo(3, 11);
	SpellSlotData spell_healing_touch = Functions::GetSpellData("Healing Touch");
	SpellSlotData spell_regrowth = Functions::GetSpellData("Regrowth");
	SpellSlotData spell_rejuvenation = Functions::GetSpellData("Rejuvenation");
	float bonusHealing = localPlayer->bonusHealing;
	//====================================================//
	float SubLevel20PENALTY = 1.0f;
	if (RegrowthLevel[spell_regrowth.rank] < 20.0f) SubLevel20PENALTY = 1.0f - (20.0f - RegrowthLevel[spell_regrowth.rank]) * 0.0375f;
	RegrowthComputed = (RegrowthValue[spell_regrowth.rank] + (bonusHealing * 0.4114 * SubLevel20PENALTY)) * (1.0f + (0.02f * GiftOfNatureRank));
	SubLevel20PENALTY = 1.0f;
	if (HealingTouchLevel[spell_healing_touch.rank] < 20.0f) SubLevel20PENALTY = 1.0f - (20.0f - HealingTouchLevel[spell_healing_touch.rank]) * 0.0375f;
	HealingTouchComputed = (HealingTouchValue[spell_healing_touch.rank] + (bonusHealing * SubLevel20PENALTY)) * (1.0f + (0.02f * GiftOfNatureRank));
	SubLevel20PENALTY = 1.0f;
	if (RejuvenationLevel[spell_rejuvenation.rank] < 20.0f) SubLevel20PENALTY = 1.0f - (20.0f - RejuvenationLevel[spell_rejuvenation.rank]) * 0.0375f;
	RejuvenationComputed = (RejuvenationValue[spell_rejuvenation.rank] + (bonusHealing * (12.0f/15.0f) * SubLevel20PENALTY)) * (1.0f + (0.02f * GiftOfNatureRank)) * (1.0f + (0.05f * ImprovedRejuvenation));
}

static void DruidAttack() {
	if (ListAI::DPSTargeting()) {}
	else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
		bool BearFormBuff = (localPlayer->hasBuff(5487) || localPlayer->hasBuff(9634));
		if (ListAI::DPSTargeting()) {}
		else if (BearFormBuff) {
			// Bear Logic
			int DemoralizingRoarIDs[5] = { 99, 1735, 9490, 9747, 9898 };
			bool DemoralizingRoarDebuff = targetUnit->hasDebuff(DemoralizingRoarIDs, 5);
			bool FrenziedRegenerationBuff = (localPlayer->hasBuff(22842) || localPlayer->hasBuff(22895) || localPlayer->hasBuff(22896));
			if (localPlayer->autoAttackGuid == 0) Functions::InteractUnit(targetUnit->Pointer, 1);
			if (Combat && localPlayer->rage < 50 && Functions::IsSpellReady("Enrage")) {
				// Enrage
				FunctionsLua::CastSpellByName("Enrage");
			}
			else if (Combat && localPlayer->prctHP < 70 && localPlayer->rage > 50 && Functions::IsSpellReady("Frenzied Regeneration")) {
				// Frenzied Regeneration
				FunctionsLua::CastSpellByName("Frenzied Regeneration");
			}
			else if (nbrCloseEnemy > 3 && !DemoralizingRoarDebuff && Functions::IsSpellReady("Demoralizing Roar")) {
				// Demoralizing Roar
				FunctionsLua::CastSpellByName("Demoralizing Roar");
			}
			else if (Functions::IsSpellReady("Bash")) {
				// Bash
				FunctionsLua::CastSpellByName("Bash");
			}
			else if ((!FrenziedRegenerationBuff || localPlayer->prctHP > 90) && nbrCloseEnemyFacing > 2 && Functions::IsSpellReady("Swipe")) {
				// Swipe
				FunctionsLua::CastSpellByName("Swipe");
			}
			else if ((!FrenziedRegenerationBuff || localPlayer->prctHP > 90) && Functions::IsSpellReady("Maul")) {
				// Maul
				FunctionsLua::CastSpellByName("Maul");
			}
		}
		else if (!BearFormBuff && HasAggro[0].size() > 2 && Functions::IsSpellReady("Bear Form")) {
			// Bear Form
			if (Functions::IsPlayerSpell("Dire Bear Form")) FunctionsLua::CastSpellByName("Dire Bear Form");
			else FunctionsLua::CastSpellByName("Bear Form");
		}
		else {
			//Specific for Hurricane cast:
			Position cluster_center = Position(0, 0, 0); int cluster_unit;
			std::tie(cluster_center, cluster_unit) = Functions::getAOETargetPos(25, 30);
			int MoonfireIDs[10] = { 8921, 8924, 8925, 8926, 8927, 8928, 8929, 9833, 9834, 9835 };
			bool MoonfireDebuff = targetUnit->hasDebuff(MoonfireIDs, 10);
			if (localPlayer->autoAttackGuid == 0) Functions::InteractUnit(targetUnit->Pointer, 1);
			if (!localPlayer->isMoving && !targetUnit->resist(SpellSchool::Nature) && (cluster_unit >= 4) && Functions::IsSpellReady("Hurricane")) {
				//Hurricane
				FunctionsLua::CastSpellByName("Hurricane");
				Functions::ClickAOE(cluster_center);
			}
			else if (IsFacing && !MoonfireDebuff && !targetUnit->resist(SpellSchool::Arcane) && targetUnit->getNbrDebuff() < 16 && !IsInGroup && Functions::IsSpellReady("Moonfire")) {
				//Moonfire
				FunctionsLua::CastSpellByName("Moonfire");
			}
			else if (!localPlayer->isMoving && (targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && !targetUnit->resist(SpellSchool::Nature) && targetUnit->getNbrDebuff() < 16 && (time(0) - EntanglingRootsTimer) > 15.0f && Functions::IsSpellReady("Entangling Roots")) {
				//Entangling Roots (PvP)
				FunctionsLua::CastSpellByName("Entangling Roots");
				if (localPlayer->isCasting()) EntanglingRootsTimer = time(0);
			}
			else if (IsFacing && !localPlayer->isMoving && !IsInGroup && !targetUnit->resist(SpellSchool::Nature) && Functions::IsSpellReady("Wrath")) {
				//Wrath
				FunctionsLua::CastSpellByName("Wrath");
			}
			else if (IsFacing && !localPlayer->isMoving && !IsInGroup && !targetUnit->resist(SpellSchool::Arcane) && Functions::IsSpellReady("Starfire")) {
				//Starfire
				FunctionsLua::CastSpellByName("Starfire");
			}
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
	bool los_heal = true; if (isParty) los_heal = !Functions::Intersect(localPlayer->position, ListUnits[indexP].position);
	float HpRatio = ListUnits[indexP].prctHP;
	int HpLost = ListUnits[indexP].hpLost;
	float distAlly = localPlayer->position.DistanceTo(ListUnits[indexP].position);
	int RejuvenationIDs[11] = { 774, 1058, 1430, 2090, 2091, 3627, 8910, 9839, 9840, 9841, 25299 };
	bool RejuvenationBuff = ListUnits[indexP].hasBuff(RejuvenationIDs, 11);
	int RegrowthIDs[9] = { 8936, 8938, 8939, 8940, 8941, 9750, 9856, 9857, 9858 };
	bool RegrowthBuff = ListUnits[indexP].hasBuff(RegrowthIDs, 9);
	// Druid Forms
	bool BearFormBuff = (localPlayer->hasBuff(5487) || localPlayer->hasBuff(9634));
	if (BearFormBuff && (HasAggro[0].size() < 2)) {
		//Disable Bear Form
		if (localPlayer->hasBuff(5487)) Functions::CancelPlayerBuff(5487);
		else Functions::CancelPlayerBuff(9634);
		return 0;
	}
	else if (!BearFormBuff && isPlayer && Combat && (localPlayer->prctHP < 70) && (HasAggro[0].size() > 0) && Functions::IsSpellReady("Barkskin")) {
		//Barkskin
		FunctionsLua::CastSpellByName("Barkskin");
		return 0;
	}
	else if (!BearFormBuff && Combat && !localPlayer->isMoving && (AoEHeal >= 4) && (distAlly < 40.0f) && Functions::IsSpellReady("Tranquility")) {
		//Tranquility
		FunctionsLua::CastSpellByName("Tranquility");
		return 0;
	}
	else if (!BearFormBuff && (HpRatio < 40) && (distAlly < 40.0f) && (RegrowthBuff || RejuvenationBuff) && Functions::IsSpellReady("Swiftmend")) {
		//Swiftmend
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Swiftmend");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!BearFormBuff && (HpLost > RegrowthComputed) && !localPlayer->isMoving && (distAlly < 40.0f) && !RegrowthBuff && Functions::IsSpellReady("Regrowth")) {
		//Regrowth
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Regrowth");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!BearFormBuff && (HpLost > HealingTouchComputed) && !localPlayer->isMoving && (distAlly < 40.0f) && Functions::IsSpellReady("Healing Touch")) {
		//Healing Touch
		localPlayer->SetTarget(healGuid);
		if(Functions::IsSpellReady("Nature's Swiftness")) FunctionsLua::CastSpellByName("Nature's Swiftness");
		FunctionsLua::CastSpellByName("Healing Touch");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!BearFormBuff && Combat && (ListUnits[indexP].prctMana < 20) && ListUnits[indexP].role == 3 && Functions::IsSpellReady("Innervate")) {
		//Innervate
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Innervate");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!BearFormBuff && (HpLost > RejuvenationComputed*0.5) && (distAlly < 40.0f) && !RejuvenationBuff && Functions::IsSpellReady("Rejuvenation")) {
		//Rejuvenation
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Rejuvenation");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	return 1;
}

void ListAI::DruidHeal() {
	int HealingTouchIDs[11] = { 5185, 5186, 5187, 5188, 5189, 6778, 8903, 9758, 9888, 9889, 25297 };
	int RegrowthIDs[9] = { 8936, 8938, 8939, 8940, 8941, 9750, 9856, 9857, 9858 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(HealingTouchIDs, 11) && (ListUnits[LastTarget].prctHP > 80))
		|| (localPlayer->isCasting(RegrowthIDs, 9) && (ListUnits[LastTarget].prctHP > 80)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	ThreadSynchronizer::RunOnMainThread([=]() {
		if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
			//Healthstone
			FunctionsLua::UseHealthstone();
			return 0;
		}
		else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
			//Healing Potion
			FunctionsLua::UseHPotion();
			return 0;
		}
		else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
			float SpellCalculTimer = 30.0f - (time(0) - current_time);
			if (SpellCalculTimer <= 0) {
				GetSpellBonusHealing();
				current_time = time(0);
			}
			int MotWIDs[9] = { 1126, 5232, 6756, 5234, 8907, 9884, 9885, 21849, 21850 }; //GotW included
			bool MotWBuff = localPlayer->hasBuff(MotWIDs, 9);
			WoWUnit* MotWPlayer = Functions::GetMissingBuff(MotWIDs, 9);
			int ThornsIDs[6] = { 467, 782, 1075, 8914, 9756, 9910 };
			bool ThornsBuff = localPlayer->hasBuff(ThornsIDs, 6);
			WoWUnit* ThornsTarget = Functions::GetMissingBuff(ThornsIDs, 6);
			WoWUnit* RemoveCurseTarget = FunctionsLua::GetGroupDispel("Curse");
			WoWUnit* CurePoisonTarget = FunctionsLua::GetGroupDispel("Poison");
			WoWUnit* deadPlayer = Functions::GetGroupDead(1);
			if (!localPlayer->isMoving && Functions::IsSpellReady("Rebirth") && (deadPlayer != NULL)) {
				//Rebirth
				localPlayer->SetTarget(deadPlayer->Guid);
				FunctionsLua::CastSpellByName("Rebirth");
			}
			else if ((MotWPlayer != NULL) && Functions::IsSpellReady("Gift of the Wild")) {
				//Gift of the Wild (Group)
				localPlayer->SetTarget(MotWPlayer->Guid);
				FunctionsLua::CastSpellByName("Gift of the Wild");
			}
			else if (!MotWBuff && Functions::IsSpellReady("Mark of the Wild")) {
				//Mark of the Wild (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Mark of the Wild");
			}
			else if ((MotWPlayer != NULL) && Functions::IsSpellReady("Mark of the Wild")) {
				//Mark of the Wild (Group)
				localPlayer->SetTarget(MotWPlayer->Guid);
				FunctionsLua::CastSpellByName("Mark of the Wild");
			}
			else if (!ThornsBuff && Functions::IsSpellReady("Thorns")) {
				//Thorns (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Thorns");
			}
			else if ((ThornsTarget != NULL) && Functions::IsSpellReady("Thorns")) {
				//Thorns (Group)
				localPlayer->SetTarget(ThornsTarget->Guid);
				FunctionsLua::CastSpellByName("Thorns");
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Curse") && Functions::IsSpellReady("Remove Curse")) {
				//Remove Curse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Remove Curse");
			}
			else if ((RemoveCurseTarget != NULL) && (localPlayer->prctMana > 25) && Functions::IsSpellReady("Remove Curse")) {
				//Remove Curse (Group)
				localPlayer->SetTarget(RemoveCurseTarget->Guid);
				FunctionsLua::CastSpellByName("Remove Curse");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Poison") && Functions::IsSpellReady("Cure Poison")) {
				//Cure Poison (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cure Poison");
			}
			else if ((CurePoisonTarget != NULL) && (localPlayer->prctMana > 25) && Functions::IsSpellReady("Cure Poison")) {
				//Cure Poison (Group)
				localPlayer->SetTarget(CurePoisonTarget->Guid);
				FunctionsLua::CastSpellByName("Cure Poison");
			}
			else if ((localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Poison") && Functions::IsSpellReady("Abolish Poison")) {
				//Abolish Poison (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Abolish Poison");
			}
			else if ((CurePoisonTarget != NULL) && (localPlayer->prctMana > 25) && Functions::IsSpellReady("Abolish Poison")) {
				//Abolish Poison (Group)
				localPlayer->SetTarget(CurePoisonTarget->Guid);
				FunctionsLua::CastSpellByName("Abolish Poison");
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
					index = (index + 1) % n;
				} while (tmp == 1 && index != (index_start - 1) % n);
				if (tmp == 1 && !passiveGroup) DruidAttack();
			}
		}
	});
}
