#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

static time_t FearTimer = time(0);
static time_t CurseGCD = time(0); // Avoid spamming if decursed

static std::string GetSpellRank(std::string txt) {
	std::string list[5] = { "Major", "Greater", "", "Lesser", "Minor" };
	for (int i = 0; i < 5; i++) {
		std::string tmp = txt + " (" + list[i] + ")";
		if (FunctionsLua::IsPlayerSpell(tmp)) return tmp;
	}
	return "";
}

bool HasSoulstone() {
	int listID[] = { 5232, 16892, 16893, 16895, 16896 };
	if (FunctionsLua::HasItem(listID, 5)) return true;
	else return false;
}

void UseSoulStone() {
	int listID[] = { 5232, 16892, 16893, 16895, 16896 };
	for (int i = 0; i < 5; i++) {
		FunctionsLua::UseItem(listID[i]);
	}
}

float GetSoulstoneCD() {
	int listID[] = { 5232, 16892, 16893, 16895, 16896 };
	float CD = FunctionsLua::GetItemCooldownDuration(listID, 5);
	return CD;
}

void ListAI::WarlockDps() {
	ThreadSynchronizer::RunOnMainThread([=]() {
		if (Combat && (localPlayer->prctHP < 40.0f) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
			//Healthstone
			FunctionsLua::UseHealthstone();
		}
		else if (Combat && (localPlayer->prctHP < 35.0f) && (FunctionsLua::GetHPotionCD() < 1.25)) {
			//Healing Potion
			FunctionsLua::UseHPotion();
		}
		else if (localPlayer->castInfo == 0 && localPlayer->channelInfo == 0 && !localPlayer->isdead && !passiveGroup) {
			ListAI::DPSTargeting();

			int DemonSkinIDs[7] = { 687, 696, 706, 1086, 11733, 11734, 11735 }; //Demon Armor included
			bool DemonSkinBuff = localPlayer->hasBuff(DemonSkinIDs, 7);
			std::string RankCreateHealthstone = GetSpellRank("Create Healthstone");
			std::string RankCreateSoulstone = GetSpellRank("Create Soulstone");
			bool SoulLinkBuff = localPlayer->hasBuff(25228);
			bool hasPet = FunctionsLua::HasPetUI();
			if (Combat && (localPlayer->prctMana < 10.0f) && (FunctionsLua::GetMPotionCD() < 1.25)) {
				//Mana Potion
				FunctionsLua::UseMPotion();
			}
			else if (!DemonSkinBuff && FunctionsLua::IsSpellReady("Demon Armor")) {
				//Demon Armor
				FunctionsLua::CastSpellByName("Demon Armor");
			}
			else if (!DemonSkinBuff && !FunctionsLua::IsPlayerSpell("Demon Armor") && FunctionsLua::IsSpellReady("Demon Skin")) {
				//Demon Skin
				FunctionsLua::CastSpellByName("Demon Skin");
			}
			else if (!SoulLinkBuff && hasPet && FunctionsLua::IsSpellReady("Soul Link")) {
				// Soul Link
				FunctionsLua::CastSpellByName("Soul Link");
			}
			else if (!hasPet && (nbrEnemyPlayer > 0) && (!Combat || FunctionsLua::IsSpellReady("Fel Domination")) && FunctionsLua::IsSpellReady("Summon Felhunter")) {
				//Summon Felhunter (PvP)
				if (Combat && FunctionsLua::IsSpellReady("Fel Domination")) FunctionsLua::CastSpellByName("Fel Domination");
				FunctionsLua::CastSpellByName("Summon Felhunter");
			}
			else if (IsInGroup && !hasPet && (!Combat || FunctionsLua::IsSpellReady("Fel Domination")) && FunctionsLua::IsSpellReady("Summon Imp")) {
				//Summon Imp (PvE)
				if (Combat && FunctionsLua::IsSpellReady("Fel Domination")) FunctionsLua::CastSpellByName("Fel Domination");
				FunctionsLua::CastSpellByName("Summon Imp");
			}
			else if (!IsInGroup && !hasPet && (!Combat || FunctionsLua::IsSpellReady("Fel Domination")) && FunctionsLua::IsSpellReady("Summon Voidwalker")) {
				//Summon Voidwalker (Solo)
				if (Combat && FunctionsLua::IsSpellReady("Fel Domination")) FunctionsLua::CastSpellByName("Fel Domination");
				FunctionsLua::CastSpellByName("Summon Voidwalker");
			}
			else if (!Combat && !localPlayer->isMoving && !FunctionsLua::HasHealthstone() && FunctionsLua::IsSpellReady(RankCreateHealthstone)) {
				//Create Healthstone
				FunctionsLua::CastSpellByName(RankCreateHealthstone + "()");
			}
			else if (!Combat && !localPlayer->isMoving && !HasSoulstone() && FunctionsLua::IsSpellReady(RankCreateSoulstone)) {
				//Create Soulstone
				FunctionsLua::CastSpellByName(RankCreateSoulstone + "()");
			}
			else if (!Combat && !localPlayer->isMoving && HasSoulstone() && (GetSoulstoneCD() < 1.0f)) {
				//Use Soulstone
				WoWUnit* healer = FunctionsLua::GetHealer();
				if(healer != NULL) localPlayer->SetTarget(healer->Guid);
				else localPlayer->SetTarget(localPlayer->Guid);
				UseSoulStone();
			}
			else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				int nbrSoulShard = FunctionsLua::GetItemCount(6265);
				int nbrAggro = HasAggro[0].size();
				int CoTonguesIDs[2] = { 1714, 11719 }; bool CoTonguesDebuff = targetUnit->hasDebuff(CoTonguesIDs, 2);
				int CoShadowIDs[2] = { 17862, 17937 }; bool CoShadowDebuff = targetUnit->hasDebuff(CoShadowIDs, 2);
				int CoAgonyIDs[6] = { 980, 1014, 6217, 11711, 11712, 11713 }; bool CoAgonyDebuff = targetUnit->hasDebuff(CoAgonyIDs, 6);
				int CoWeakness[6] = { 702, 1108, 6205, 7646, 11707, 11708 }; bool CoWeaknessDebuff = targetUnit->hasDebuff(CoWeakness, 6);
				int CorruptionIDs[7] = { 172, 6222, 6223, 7648, 11671, 11672, 25311 }; bool CorruptionDebuff = targetUnit->hasDebuff(CorruptionIDs, 7);
				int SiphonLifeIDs[4] = { 18265, 18879, 18880, 18881 }; bool SiphonLifeDebuff = targetUnit->hasDebuff(SiphonLifeIDs, 4);
				int ImmolateIDs[8] = { 348, 707, 1094, 2941, 11665, 11667, 11668, 25309 }; bool ImmolateDebuff = targetUnit->hasDebuff(ImmolateIDs, 8);
				//Specific for Rain of Fire cast:
				Position cluster_center = Position(0, 0, 0); int cluster_unit;
				std::tie(cluster_center, cluster_unit) = Functions::getAOETargetPos(25, 30);
				if ((targetUnit->flags & UNIT_FLAG_IN_COMBAT) && hasPet) {
					Functions::LuaCall("PetAttack()");
				}
				if ((localPlayer->prctHP < 40.0f) && targetPlayer && !targetUnit->isCrowdControlled() && FunctionsLua::IsSpellReady("Death Coil")) {
					//Death Coil (PvP)
					FunctionsLua::CastSpellByName("Death Coil");
				}
				else if (!localPlayer->isMoving && (nbrCloseEnemy >= 2) && FunctionsLua::IsSpellReady("Howl of Terror")) {
					//Howl of Terror
					FunctionsLua::CastSpellByName("Howl of Terror");
				}
				else if (!CoTonguesDebuff && targetPlayer && (targetUnit->getNbrDebuff() < 16) && (time(0) - CurseGCD) >= 5.0f && FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Curse of Tongues")) {
					//Curse of Tongues (PvP -> Caster)
					FunctionsLua::CastSpellByName("Curse of Tongues");
					CurseGCD = time(0);
				}
				else if (!CoWeaknessDebuff && targetPlayer && (targetUnit->targetGuid == localPlayer->Guid) && (targetUnit->getNbrDebuff() < 16) && (time(0) - CurseGCD) >= 5.0f && !FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Curse of Weakness")) {
					//Curse of Weakness (PvP -> Melee)
					FunctionsLua::CastSpellByName("Curse of Weakness");
					CurseGCD = time(0);
				}
				else if (!localPlayer->isMoving && targetPlayer && !targetUnit->isCrowdControlled() && (time(0) - FearTimer) >= 15.0f && FunctionsLua::IsSpellReady("Fear")) {
					//Fear (PvP)
					FunctionsLua::CastSpellByName("Fear");
					if (localPlayer->isCasting()) FearTimer = time(0);
				}
				else if (!localPlayer->isMoving && targetPlayer && (targetUnit->level >= localPlayer->level-10) && !targetUnit->isCrowdControlled() && FunctionsLua::IsSpellReady("Inferno")) {
					//Inferno (PvP)
					if (hasPet) Functions::LuaCall("PetDismiss()");
					FunctionsLua::CastSpellByName("Inferno");
					Functions::ClickAOE(targetUnit->position);
				}
				else if (!localPlayer->isMoving && (cluster_unit >= 6) && FunctionsLua::IsSpellReady("Inferno")) {
					//Inferno (AoE)
					if (hasPet) Functions::LuaCall("PetDismiss()");
					FunctionsLua::CastSpellByName("Inferno");
					Functions::ClickAOE(cluster_center);
				}
				else if (!localPlayer->isMoving && (localPlayer->prctHP > 70.0f) && (nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Hellfire")) {
					//Hellfire
					FunctionsLua::CastSpellByName("Hellfire");
				}
				else if (!localPlayer->isMoving && (cluster_unit >= 4) && FunctionsLua::IsSpellReady("Rain of Fire")) {
					//Rain of Fire
					FunctionsLua::CastSpellByName("Rain of Fire");
					Functions::ClickAOE(cluster_center);
				}
				else if (!CoShadowDebuff && (targetUnit->level == -1) && (targetUnit->getNbrDebuff() < 16) && (time(0) - CurseGCD) >= 5.0f && FunctionsLua::IsSpellReady("Curse of Shadow")) {
					//Curse of Shadow (Boss)
					FunctionsLua::CastSpellByName("Curse of Shadow");
					CurseGCD = time(0);
				}
				else if (!localPlayer->isMoving && (localPlayer->prctHP < 40.0f) && FunctionsLua::IsSpellReady("Drain Life")) {
					//Drain Life
					FunctionsLua::CastSpellByName("Drain Life");
				}
				else if (!CoAgonyDebuff && targetUnit->getNbrDebuff() < 16 && !CoTonguesDebuff && targetPlayer && (time(0) - CurseGCD) >= 5.0f && FunctionsLua::IsSpellReady("Curse of Agony")) {
					//Curse of Agony (PvP)
					FunctionsLua::CastSpellByName("Curse of Agony");
					CurseGCD = time(0);
				}
				else if (!CorruptionDebuff && targetUnit->getNbrDebuff() < 16 && targetPlayer && FunctionsLua::IsSpellReady("Corruption")) {
					//Corruption (PvP)
					FunctionsLua::CastSpellByName("Corruption");
				}
				else if (!SiphonLifeDebuff && targetUnit->getNbrDebuff() < 16 && targetPlayer && FunctionsLua::IsSpellReady("Siphon Life")) {
					//Siphon Life (PvP)
					FunctionsLua::CastSpellByName("Siphon Life");
				}
				else if (!localPlayer->isMoving && !ImmolateDebuff && targetUnit->getNbrDebuff() < 16 && targetPlayer && FunctionsLua::IsSpellReady("Immolate")) {
					//Immolate (PvP)
					FunctionsLua::CastSpellByName("Immolate");
				}
				else if (!localPlayer->isMoving && (nbrSoulShard < 6) && (targetUnit->prctHP < 15.0f) && FunctionsLua::IsSpellReady("Drain Soul")) {
					//Drain Soul
					FunctionsLua::CastSpellByName("Drain Soul");
				}
				else if (!localPlayer->isMoving && (targetUnit->prctMana > 33.0f) && targetPlayer && FunctionsLua::IsSpellReady("Drain Mana")) {
					//Drain Mana (PvP)
					FunctionsLua::CastSpellByName("Drain Mana");
				}
				else if (IsFacing && !localPlayer->isMoving && FunctionsLua::IsSpellReady("Shadow Bolt")) {
					//Shadow Bolt
					FunctionsLua::CastSpellByName("Shadow Bolt");
				}
				else if (!localPlayer->isMoving && (localPlayer->prctHP > 40.0f) && (localPlayer->prctMana < 10.0f) && FunctionsLua::IsSpellReady("Life Tap")) {
					//Life Tap
					FunctionsLua::CastSpellByName("Life Tap");
				}
				else if (IsFacing && !localPlayer->isMoving && FunctionsLua::HasWandEquipped() && !FunctionsLua::IsAutoRepeatAction(FunctionsLua::GetSlot("Shoot"))) {
					//Wand
					FunctionsLua::CastSpellByName("Shoot");
				}
			}
		}
	});
}
