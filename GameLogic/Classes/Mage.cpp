#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static std::string GetSpellRank(std::string txt) {
	std::string list[4] = { "Ruby", "Citrine", "Jade", "Agate" };
	for (int i = 0; i < 4; i++) {
		if (FunctionsLua::IsPlayerSpell(txt + " " + list[i])) return (txt + " " + list[i]);
	}
	return "";
}

bool HasManaStone() {
	int listID[4] = { 5514, 5513, 8007, 8008 };
	if (FunctionsLua::HasItem(listID, 4)) return true;
	else return false;
}

void UseManaStone() {
	int listID[4] = { 5514, 5513, 8007, 8008 };
	for (int i = 0; i < 4; i++) {
		FunctionsLua::UseItem(listID[i]);
	}
}

float GetManaStoneCD() {
	int listID[4] = { 5514, 5513, 8007, 8008 };
	for (int i = 0; i < 4; i++) {
		float CD = FunctionsLua::GetItemCooldownDuration(listID[i]);
		if(CD < 999) return CD;
	}
	return 999;
}

void ListAI::MageDps() {
	time_t PolymorphTimer = 15 - (time(0) - current_time);
	if (PolymorphTimer < 0) PolymorphTimer = 0;
	int EvocationIDs[1] = { 12051 };
	if (localPlayer->isChanneling(EvocationIDs, 1) && (localPlayer->prctMana > 80)) {
		ThreadSynchronizer::pressKey(0x28);
		ThreadSynchronizer::releaseKey(0x28);
	}
	else if (localPlayer->castInfo == 0 && localPlayer->channelInfo == 0 && !localPlayer->isdead && !passiveGroup) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			bool IsStunned = localPlayer->flags & UNIT_FLAG_STUNNED;
			bool IsConfused = localPlayer->flags & UNIT_FLAG_CONFUSED;
			int FrostArmorIDs[7] = { 168, 7300, 7301, 7302, 7320, 10219, 10220 };
			bool FrostArmorBuff = localPlayer->hasBuff(FrostArmorIDs, 7);
			int MageArmorIDs[3] = { 6117, 22782, 22783 };
			bool MageArmorBuff = localPlayer->hasBuff(MageArmorIDs, 3);
			int IceBarrierIDs[4] = { 11426, 13031, 13032, 13033 };
			bool IceBarrierBuff = localPlayer->hasBuff(IceBarrierIDs, 4);
			int ManaShieldIDs[6] = { 1463, 8494, 8495, 10191, 10192, 10193 };
			bool ManaShieldBuff = localPlayer->hasBuff(ManaShieldIDs, 6);
			int ArcaneIntellectIDs[5] = { 1459, 1460, 1461, 10156, 10157 };
			bool ArcaneIntellectBuff = localPlayer->hasBuff(ArcaneIntellectIDs, 5);
			int PolymorphIDs[6] = { 118, 12824, 12825, 12826, 28271, 28272 };
			bool PolymorphDebuff = false;
			if (targetUnit != NULL) PolymorphDebuff = targetUnit->hasDebuff(PolymorphIDs, 6);
			WoWUnit* ArcaneIntellectTarget = Functions::GetMissingBuff(ArcaneIntellectIDs, 5);

			//Specific for Blizzard cast:
			Position cluster_center = Position(0, 0, 0); int cluster_unit;
			std::tie(cluster_center, cluster_unit) = Functions::getAOETargetPos(25, 30);

			WoWUnit* RemoveCurseTarget = FunctionsLua::GetGroupDispel("Curse");
			std::string RankConjureMana = GetSpellRank("Conjure Mana");

			ListAI::DPSTargeting();

			if (IsStunned && FunctionsLua::IsSpellReady("Blink")) {
				//Blink
				FunctionsLua::CastSpellByName("Blink");
			}
			else if ((IsConfused || (localPlayer->prctHP < 20 && (HasAggro[0].size() > 0)) || (IsStunned && nbrEnemyPlayer > 0) || localPlayer->getNbrDebuff() >= 4) && FunctionsLua::IsSpellReady("Ice Block")) {
				//Ice Block
				FunctionsLua::CastSpellByName("Ice Block");
			}
			else if (!FrostArmorBuff && (mapID == 489 || mapID == 529 || !FunctionsLua::IsPlayerSpell("Mage Armor")) && FunctionsLua::IsSpellReady("Frost Armor")) {
				//Frost|Ice Armor (PvP -> BG)
				FunctionsLua::CastSpellByName("Ice Armor");
				FunctionsLua::CastSpellByName("Frost Armor");
			}
			else if (!MageArmorBuff && !FrostArmorBuff && (mapID != 489 && mapID != 529) && FunctionsLua::IsSpellReady("Mage Armor")) {
				//Mage Armor (PvE)
				FunctionsLua::CastSpellByName("Mage Armor");
			}
			else if (!Combat && !ArcaneIntellectBuff && FunctionsLua::IsSpellReady("Arcane Intellect")) {
				//Arcane Intellect (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Arcane Intellect");
			}
			else if (!Combat && (ArcaneIntellectTarget != NULL) && FunctionsLua::IsSpellReady("Arcane Intellect")) {
				//Arcane Intellect (group)
				localPlayer->SetTarget(ArcaneIntellectTarget->Guid);
				FunctionsLua::CastSpellByName("Arcane Intellect");
			}
			else if (!Combat && !localPlayer->isMoving && !HasManaStone() && FunctionsLua::IsSpellReady(RankConjureMana)) {
				//Conjure Mana (stone)
				FunctionsLua::CastSpellByName(RankConjureMana);
			}
			else if (!Combat && !localPlayer->isMoving && (FunctionsLua::HasDrink() == 0) && FunctionsLua::IsSpellReady("Conjure Water")) {
				//Conjure Water
				FunctionsLua::CastSpellByName("Conjure Water");
			}
			else if (!IceBarrierBuff && FunctionsLua::IsSpellReady("Ice Barrier")) {
				//Ice Barrier
				FunctionsLua::CastSpellByName("Ice Barrier");
			}
			else if ((localPlayer->prctHP < 25) && (localPlayer->prctMana > 50) && !ManaShieldBuff && FunctionsLua::IsSpellReady("Mana Shield")) {
				//Mana Shield
				FunctionsLua::CastSpellByName("Mana Shield");
			}
			else if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
				//Healthstone
				FunctionsLua::UseHealthstone();
			}
			else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
				//Healing Potion
				FunctionsLua::UseHPotion();
			}
			else if (Combat && (localPlayer->prctMana < 15) && (GetManaStoneCD() < 1.25)) {
				//Mana Stone
				UseManaStone();
			}
			else if (Combat && !localPlayer->isMoving && (localPlayer->prctMana < 15) && ((nbrCloseEnemy == 0) || (HasAggro[0].size() == 0)) && FunctionsLua::IsSpellReady("Evocation")) {
				//Evocation
				FunctionsLua::CastSpellByName("Evocation");
			}
			else if (Combat && (localPlayer->prctMana < 10) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if (nbrEnemyPlayer == 0 && FunctionsLua::GetUnitDispel("player", "Curse") && FunctionsLua::IsSpellReady("Remove Lesser Curse")) {
				//Remove Lesser Curse (self)
				localPlayer->SetTarget(localPlayer->Guid);
				FunctionsLua::CastSpellByName("Remove Lesser Curse");
				if(Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if (nbrEnemyPlayer == 0 && (RemoveCurseTarget != NULL) && FunctionsLua::IsSpellReady("Remove Lesser Curse")) {
				//Remove Lesser Curse (group)
				localPlayer->SetTarget(RemoveCurseTarget->Guid);
				FunctionsLua::CastSpellByName("Remove Lesser Curse");
				if(Combat) Functions::LuaCall("TargetLastEnemy()");
			}
			else if (targetUnit != NULL && targetUnit->attackable) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				bool targetStunned = targetUnit->flags & UNIT_FLAG_CONFUSED;
				bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
				if (Combat && playerSpec == 2 && targetPlayer && FunctionsLua::IsSpellReady("Cold Snap") && !FunctionsLua::IsSpellReady("Frost Nova") && !FunctionsLua::IsSpellReady("Ice Block")) {
					FunctionsLua::CastSpellByName("Cold Snap");
				}
				else if ((nbrCloseEnemy >= 3 || (nbrCloseEnemyFacing >= 1 && targetPlayer) || (nbrCloseEnemy >= 1 && !IsInGroup)) && FunctionsLua::IsSpellReady("Frost Nova")) {
					//Frost Nova
					FunctionsLua::CastSpellByName("Frost Nova");
				}
				else if ((playerSpec == 1) && (nbrCloseEnemy >= 3 || (nbrCloseEnemyFacing >= 1 && targetPlayer) || (nbrCloseEnemy >= 1 && !IsInGroup)) && FunctionsLua::IsSpellReady("Blast Wave")) {
					//Blast Wave
					FunctionsLua::CastSpellByName("Blast Wave");
				}
				else if ((nbrCloseEnemyFacing >= 3 || (nbrCloseEnemyFacing >= 1 && targetPlayer) || (nbrCloseEnemyFacing >= 1 && !IsInGroup)) && FunctionsLua::IsSpellReady("Cone of Cold")) {
					//Cone of Cold
					FunctionsLua::CastSpellByName("Cone of Cold");
				}
				else if (IsFacing && targetUnit->channelInfo > 0 && FunctionsLua::IsSpellReady("Counterspell")) {
					//Counter Spell
					FunctionsLua::CastSpellByName("Counterspell");
				}
				else if ((ccTarget != NULL) && (PolymorphTimer == 0) && !(ccTarget->flags & UNIT_FLAG_CONFUSED) && FunctionsLua::IsSpellReady("Polymorph")) {
					//Polymorph (second target)
					WoWUnit* firstTarget = targetUnit;
					localPlayer->SetTarget(ccTarget->Guid);
					FunctionsLua::CastSpellByName("Polymorph");
					if(localPlayer->isCasting()) current_time = time(0);
					localPlayer->SetTarget(firstTarget->Guid);
				}
				else if (!localPlayer->isMoving && (cluster_unit >= 4) && (playerSpec == 1 || localPlayer->level < 20) && FunctionsLua::IsSpellReady("Flamestrike")) {
					//Flamestrike
					FunctionsLua::CastSpellByName("Flamestrike");
					Functions::ClickAOE(cluster_center);
				}
				else if (!localPlayer->isMoving && (cluster_unit >= 4) && FunctionsLua::IsSpellReady("Blizzard")) {
					//Blizzard
					FunctionsLua::CastSpellByName("Blizzard");
					Functions::ClickAOE(cluster_center);
				}
				else if ((localPlayer->speed > 0 || localPlayer->level < 20) && (nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Arcane Explosion")) {
					//Arcane Explosion
					FunctionsLua::CastSpellByName("Arcane Explosion");
				}
				else if (IsFacing && (localPlayer->speed > 0) && FunctionsLua::IsSpellReady("Fire Blast")) {
					//Fire Blast (Movement)
					FunctionsLua::CastSpellByName("Fire Blast");
				}
				else if (IsFacing && !localPlayer->isMoving && (playerSpec == 1) && (FunctionsLua::GetStackDebuff("target", "Interface\\Icons\\Spell_Fire_Soulburn") < 5) && FunctionsLua::IsSpellReady("Scorch")) {
					//Scorch
					FunctionsLua::CastSpellByName("Scorch");
				}
				else if ((playerSpec == 1) && FunctionsLua::UnitIsElite("target") && FunctionsLua::IsSpellReady("Combustion")) {
					//Combustion
					FunctionsLua::CastSpellByName("Combustion");
				}
				else if ((playerSpec == 1) && FunctionsLua::GetUnitBuff("player", "Interface\\Icons\\Spell_Fire_SealOfFire") && FunctionsLua::IsSpellReady("Pyroblast")) {
					//Pyroblast
					FunctionsLua::CastSpellByName("Pyroblast");
				}
				else if (IsFacing && !localPlayer->isMoving && (playerSpec == 1) && FunctionsLua::IsSpellReady("Fireball")) {
					//Fireball
					FunctionsLua::CastSpellByName("Fireball");
				}
				else if (IsFacing && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Frostbolt")) {
					//Frostbolt
					FunctionsLua::CastSpellByName("Frostbolt");
				}
				else if (IsFacing && !localPlayer->isMoving && FunctionsLua::HasWandEquipped() && !FunctionsLua::IsAutoRepeatAction(FunctionsLua::GetSlot("Shoot"))) {
					//Wand
					FunctionsLua::CastSpellByName("Shoot");
				}
			}
		} );
	}
}