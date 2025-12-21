#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static void DruidAttack() {
	int CatFormIDs[1] = { 768 }; bool CatFormBuff = localPlayer->hasBuff(CatFormIDs, 1);
	ListAI::DPSTargeting();
	if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
		if (CatFormBuff) {
			// Kitty Logic
			int RakeIDs[4] = { 1822, 1823, 1824, 9904 };
			bool RakeDebuff = targetUnit->hasDebuff(RakeIDs, 4);
			int RipIDs[6] = { 1079, 9492, 9493, 9752, 9894, 9896 };
			bool RipDebuff = targetUnit->hasDebuff(RipIDs, 6);
			int FaerieFireIDs[4] = { 16857, 17390, 17391, 17392 };
			bool FaerieFireDebuff = targetUnit->hasDebuff(FaerieFireIDs, 4);
			int ProwlIDs[3] = { 5215, 6783, 9913 };
			bool ProwlBuff = localPlayer->hasBuff(ProwlIDs, 3);
			int ComboPoints = FunctionsLua::GetComboPoints();
			if (!ProwlBuff && !FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
			if (!Combat && !ProwlBuff && FunctionsLua::IsSpellReady("Prowl")) {
				// Prowl -> Not in PvE raids
				FunctionsLua::CastSpellByName("Prowl");
			}
			else if (Combat && (distTarget > 12) && (localPlayer->speed < 7) && FunctionsLua::IsSpellReady("Dash")) {
				// Dash
				FunctionsLua::CastSpellByName("Dash");
			}
			else if (ProwlBuff && localPlayer->isBehind(targetUnit) && FunctionsLua::IsSpellReady("Ravage")) {
				// Ravage
				FunctionsLua::CastSpellByName("Ravage");
			}
			else if (ProwlBuff && FunctionsLua::IsSpellReady("Pounce")) {
				// Pounce
				FunctionsLua::CastSpellByName("Pounce");
			}
			else if (!ProwlBuff && IsInGroup && !(targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && (targetUnit->targetGuid == localPlayer->Guid) && FunctionsLua::IsSpellReady("Cower")) {
				// Cower
				FunctionsLua::CastSpellByName("Cower");
			}
			else if (!ProwlBuff && ComboPoints >= 4 && !RipDebuff && targetUnit->getNbrDebuff() < 16 && (targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && FunctionsLua::IsSpellReady("Rip")) {
				// Rip (PvP)
				FunctionsLua::CastSpellByName("Rip");
			}
			else if (!ProwlBuff && ComboPoints >= 4 && FunctionsLua::IsSpellReady("Ferocious Bite")) {
				// Ferocious Bite
				FunctionsLua::CastSpellByName("Ferocious Bite");
			}
			else if (!ProwlBuff && localPlayer->isBehind(targetUnit) && FunctionsLua::IsSpellReady("Shred")) {
				// Shred
				FunctionsLua::CastSpellByName("Shred");
			}
			else if (!ProwlBuff && !FaerieFireDebuff && targetUnit->getNbrDebuff() < 16 && FunctionsLua::IsSpellReady("Faerie Fire (Feral)")) {
				// Faerie Fire (Feral)
				FunctionsLua::CastSpellByName("Faerie Fire (Feral)()");
			}
			else if (!ProwlBuff && !RakeDebuff && targetUnit->getNbrDebuff() < 16 && (targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && FunctionsLua::IsSpellReady("Rake")) {
				// Rake (PvP)
				FunctionsLua::CastSpellByName("Rake");
			}
			else if (!ProwlBuff && FunctionsLua::IsSpellReady("Claw")) {
				// Claw
				FunctionsLua::CastSpellByName("Claw");
			}
		}
		else if (!CatFormBuff && FunctionsLua::IsSpellReady("Cat Form")) {
			// Kitty Form
			FunctionsLua::CastSpellByName("Cat Form");
		}
		else {
			// Human Logic
			time_t EntanglingRootsTimer = 15 - (time(0) - current_time);
			if (EntanglingRootsTimer < 0) EntanglingRootsTimer = 0;
			//Specific for Hurricane cast:
			Position cluster_center = Position(0, 0, 0); int cluster_unit;
			std::tie(cluster_center, cluster_unit) = Functions::getAOETargetPos(25, 30);
			int MoonfireIDs[10] = { 8921, 8924, 8925, 8926, 8927, 8928, 8929, 9833, 9834, 9835 };
			bool MoonfireDebuff = targetUnit->hasDebuff(MoonfireIDs, 10);
			if (!FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
			if (!localPlayer->isMoving && (cluster_unit >= 4) && FunctionsLua::IsSpellReady("Hurricane")) {
				//Hurricane
				FunctionsLua::CastSpellByName("Hurricane");
				Functions::ClickAOE(cluster_center);
			}
			else if (IsFacing && !MoonfireDebuff && targetUnit->getNbrDebuff() < 16 && !IsInGroup && FunctionsLua::IsSpellReady("Moonfire")) {
				//Moonfire
				FunctionsLua::CastSpellByName("Moonfire");
			}
			else if (!localPlayer->isMoving && (targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED) && (EntanglingRootsTimer == 0) && targetUnit->getNbrDebuff() < 16 && FunctionsLua::IsSpellReady("Entangling Roots")) {
				//Entangling Roots (PvP)
				FunctionsLua::CastSpellByName("Entangling Roots");
				if (localPlayer->isCasting()) current_time = time(0);
			}
			else if (IsFacing && !localPlayer->isMoving && !IsInGroup && FunctionsLua::IsSpellReady("Wrath")) {
				//Wrath
				FunctionsLua::CastSpellByName("Wrath");
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
	float distAlly = localPlayer->position.DistanceTo(ListUnits[indexP].position);
	int RejuvenationIDs[11] = { 774, 1058, 1430, 2090, 2091, 3627, 8910, 9839, 9840, 9841, 25299 };
	bool RejuvenationBuff = ListUnits[indexP].hasBuff(RejuvenationIDs, 11);
	int RegrowthIDs[9] = { 8936, 8938, 8939, 8940, 8941, 9750, 9856, 9857, 9858 };
	bool RegrowthBuff = ListUnits[indexP].hasBuff(RegrowthIDs, 9);
	int CatFormIDs[1] = { 768 }; bool CatFormBuff = localPlayer->hasBuff(CatFormIDs, 1);
	if (CatFormBuff && (HpRatio < 60.0f) && (localPlayer->prctMana > 50.0f)) {
		//Disable Cat Form
		FunctionsLua::CastSpellByName("Cat Form");
		return 0;
	}
	else if (!CatFormBuff && isPlayer && Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
		//Healthstone
		FunctionsLua::UseHealthstone();
		return 0;
	}
	else if (!CatFormBuff && isPlayer && Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
		//Healing Potion
		FunctionsLua::UseHPotion();
		return 0;
	}
	else if (!CatFormBuff && isPlayer && Combat && (HasAggro[0].size() > 0) && (localPlayer->prctHP < 70) && FunctionsLua::IsSpellReady("Barkskin")) {
		//Barkskin
		FunctionsLua::CastSpellByName("Barkskin");
		return 0;
	}
	else if (!CatFormBuff && !localPlayer->isMoving && Combat && (AoEHeal >= 4) && (distAlly < 40.0f) && FunctionsLua::IsSpellReady("Tranquility")) {
		//Tranquility
		FunctionsLua::CastSpellByName("Tranquility");
		return 0;
	}
	else if ((HpRatio < 40) && (distAlly < 40.0f) && (RegrowthBuff || RejuvenationBuff) && FunctionsLua::IsSpellReady("Swiftmend")) {
		//Swiftmend
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Swiftmend");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!CatFormBuff && !localPlayer->isMoving && (HpRatio < 60) && (distAlly < 40.0f) && !RegrowthBuff && FunctionsLua::IsSpellReady("Regrowth")) {
		//Regrowth
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Regrowth");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!CatFormBuff && !localPlayer->isMoving && (HpRatio < 40) && (distAlly < 40.0f) && FunctionsLua::IsSpellReady("Healing Touch")) {
		//Healing Touch
		localPlayer->SetTarget(healGuid);
		if (FunctionsLua::IsSpellReady("Nature's Swiftness")) FunctionsLua::CastSpellByName("Nature's Swiftness");
		FunctionsLua::CastSpellByName("Healing Touch");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	else if (!CatFormBuff && (HpRatio < 85) && (distAlly < 40.0f) && !RejuvenationBuff && FunctionsLua::IsSpellReady("Rejuvenation")) {
		//Rejuvenation
		localPlayer->SetTarget(healGuid);
		FunctionsLua::CastSpellByName("Rejuvenation");
		LastTarget = indexP;
		if (!los_heal) Moving = 5;
		return 0;
	}
	return 1;
}

void ListAI::DruidFeralCat() {
	int HealingTouchIDs[11] = { 5185, 5186, 5187, 5188, 5189, 6778, 8903, 9758, 9888, 9889, 25297 };
	int RegrowthIDs[9] = { 8936, 8938, 8939, 8940, 8941, 9750, 9856, 9857, 9858 };
	if ((ListUnits.size() > LastTarget) && ((localPlayer->isCasting(HealingTouchIDs, 11) && (ListUnits[LastTarget].prctHP > 80))
		|| (localPlayer->isCasting(RegrowthIDs, 9) && (ListUnits[LastTarget].prctHP > 80)))) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	else if ((localPlayer->castInfo == 0) && (localPlayer->channelInfo == 0) && !localPlayer->isdead) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			int MotWIDs[9] = { 1126, 5232, 6756, 5234, 8907, 9884, 9885, 21849, 21850 }; //GotW included
			bool MotWBuff = localPlayer->hasBuff(MotWIDs, 9);
			WoWUnit* MotWPlayer = Functions::GetMissingBuff(MotWIDs, 9);
			int ThornsIDs[6] = { 467, 782, 1075, 8914, 9756, 9910 };
			bool ThornsBuff = localPlayer->hasBuff(ThornsIDs, 6);
			WoWUnit* ThornsTarget = Functions::GetMissingBuff(ThornsIDs, 6);
			WoWUnit* RemoveCurseTarget = FunctionsLua::GetGroupDispel("Curse");
			WoWUnit* CurePoisonTarget = FunctionsLua::GetGroupDispel("Poison");
			WoWUnit* deadPlayer = Functions::GetGroupDead(1);
			int CatFormIDs[1] = { 768 }; bool CatFormBuff = localPlayer->hasBuff(CatFormIDs, 1);
			if (CatFormBuff && !Combat && (targetUnit == NULL || !targetUnit->attackable || targetUnit->isdead) && localPlayer->prctMana > 70.0f) {
				//Kitty Form
				FunctionsLua::CastSpellByName("Cat Form");
			}
			else if (!CatFormBuff && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Rebirth") && (deadPlayer != NULL)) {
				//Rebirth
				localPlayer->SetTarget(deadPlayer->Guid);
				FunctionsLua::CastSpellByName("Rebirth");
			}
			else if (!CatFormBuff && (MotWPlayer != NULL) && FunctionsLua::IsSpellReady("Gift of the Wild")) {
				//Gift of the Wild (Group)
				localPlayer->SetTarget(MotWPlayer->Guid);
				FunctionsLua::CastSpellByName("Gift of the Wild");
			}
			else if (!CatFormBuff && !MotWBuff && FunctionsLua::IsSpellReady("Mark of the Wild")) {
				//Mark of the Wild (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Mark of the Wild");
			}
			else if (!CatFormBuff && (MotWPlayer != NULL) && FunctionsLua::IsSpellReady("Mark of the Wild")) {
				//Mark of the Wild (Group)
				localPlayer->SetTarget(MotWPlayer->Guid);
				FunctionsLua::CastSpellByName("Mark of the Wild");
			}
			else if (!CatFormBuff && !ThornsBuff && FunctionsLua::IsSpellReady("Thorns")) {
				//Thorns (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Thorns");
			}
			else if (!CatFormBuff && (ThornsTarget != NULL) && FunctionsLua::IsSpellReady("Thorns")) {
				//Thorns (Group)
				localPlayer->SetTarget(ThornsTarget->Guid);
				FunctionsLua::CastSpellByName("Thorns");
			}
			else if (!CatFormBuff && Combat && (localPlayer->prctMana < 10) && FunctionsLua::IsSpellReady("Innervate")) {
				//Innervate
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Innervate");
			}
			else if (!CatFormBuff && Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if (!CatFormBuff && (localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Curse") && FunctionsLua::IsSpellReady("Remove Curse")) {
				//Remove Curse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Remove Curse");
			}
			else if (!CatFormBuff && (RemoveCurseTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Remove Curse")) {
				//Remove Curse (Group)
				localPlayer->SetTarget(RemoveCurseTarget->Guid);
				FunctionsLua::CastSpellByName("Remove Curse");
			}
			else if (!CatFormBuff && (localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Poison") && FunctionsLua::IsSpellReady("Cure Poison")) {
				//Cure Poison (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Cure Poison");
			}
			else if (!CatFormBuff && (CurePoisonTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Cure Poison")) {
				//Cure Poison (Group)
				localPlayer->SetTarget(CurePoisonTarget->Guid);
				FunctionsLua::CastSpellByName("Cure Poison");
			}
			else if (!CatFormBuff && (localPlayer->prctMana > 25) && FunctionsLua::GetUnitDispel("player", "Poison") && FunctionsLua::IsSpellReady("Abolish Poison")) {
				//Abolish Poison (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Abolish Poison");
			}
			else if (!CatFormBuff && (CurePoisonTarget != NULL) && (localPlayer->prctMana > 25) && FunctionsLua::IsSpellReady("Abolish Poison")) {
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
		});
	}
}
