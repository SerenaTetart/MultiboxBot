#include "ListAI.h"

#include <iostream>
#include <ctime>

void ListAI::WarriorTank() {
	if (localPlayer->castInfo == 0 && localPlayer->channelInfo == 0 && !localPlayer->isdead && !passiveGroup) {
		ThreadSynchronizer::RunOnMainThread([=]() {
			if ((Leader == NULL) || (Leader->Guid != localPlayer->Guid) || tankAutoFocus) {
				bool targetFocusingTank = false;
				if (targetUnit != NULL) {
					for (int i = 1; i <= NumGroupMembers; i++) {
						if (GroupMember[i] != NULL && targetUnit->targetGuid == GroupMember[i]->Guid && GroupMember[i]->role == 0) {
							targetFocusingTank = true;
							break;
						}
					}
				}
				if (targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable || targetFocusingTank) {
					bool foundTarget = false;
					for (int i = NumGroupMembers; i >= 0; i--) {
						if (GroupMember[i] != NULL && GroupMember[i]->role != 0 && HasAggro[i].size() > 0) {
							localPlayer->SetTarget(HasAggro[i][0]->Guid);
							foundTarget = true;
							break;
						}
					}
					if ((targetUnit == NULL || targetUnit->isdead || !targetUnit->attackable) && !foundTarget) {
						for (int i = NumGroupMembers; i >= 0; i--) { //Tank also
							if (HasAggro[i].size() > 0) {
								localPlayer->SetTarget(HasAggro[i][0]->Guid);
								break;
							}
						}
					}
				}
			}
			int BattleShoutIDs[7] = { 6673, 5242, 6192, 11549, 11550, 11551, 25289 }; bool BattleShoutBuff = localPlayer->hasBuff(BattleShoutIDs, 7);
			int DemoralizingShoutIDs[5] = { 1160, 6190, 11554, 11555, 11556 }; bool DemoralizingShoutDebuff = localPlayer->hasBuff(DemoralizingShoutIDs, 5);
			if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
				//Healthstone
				FunctionsLua::UseItem("Healthstone");
			}
			else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
				//Healing Potion
				FunctionsLua::UseItem("Healing Potion");
			}
			else if (Combat && (localPlayer->prctHP < 40) && FunctionsLua::IsSpellReady("Last Stand")) {
				//Last Stand
				FunctionsLua::CastSpellByName("Last Stand");
			}
			else if (Combat && (localPlayer->prctHP < 25) && FunctionsLua::IsSpellReady("Shield Wall")) {
				//Shield Wall
				FunctionsLua::CastSpellByName("Shield Wall");
			}
			else if ((nbrCloseEnemy >= 4) && FunctionsLua::IsSpellReady("Intimidating Shout")) {
				//Intimidating Shout
				FunctionsLua::CastSpellByName("Intimidating Shout");
			}
			else if (Combat && !BattleShoutBuff && FunctionsLua::IsSpellReady("Battle Shout")) {
				//Battle Shout
				FunctionsLua::CastSpellByName("Battle Shout");
			}
			else if ((nbrCloseEnemy >= 3) && !DemoralizingShoutDebuff && FunctionsLua::IsSpellReady("Demoralizing Shout")) {
				//Demoralizing Shout
				FunctionsLua::CastSpellByName("Demoralizing Shout");
			}
			else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
				bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
				bool BattleStance = FunctionsLua::GetShapeshiftFormInfo(1);
				bool DefensiveStance = FunctionsLua::GetShapeshiftFormInfo(2);
				bool BerserkerStance = FunctionsLua::GetShapeshiftFormInfo(3);
				if (!FunctionsLua::IsCurrentAction(FunctionsLua::GetSlot("Attack"))) FunctionsLua::CastSpellByName("Attack");
				if (BattleStance) {
					int HamstringIDs[3] = { 1715, 7372, 7373 };
					bool HamstringDebuff = targetUnit->hasDebuff(HamstringIDs, 3);
					int ThunderClapIDs[5] = { 6343, 8198, 8205, 11580, 11581 };
					bool ThunderClapDebuff = targetUnit->hasDebuff(ThunderClapIDs, 5);
					int RendIDs[7] = { 772, 6546, 6547, 6548, 11572, 11573, 11574 };
					bool RendDebuff = targetUnit->hasDebuff(RendIDs, 7);
					if ((distTarget < 25.0f) && FunctionsLua::IsSpellReady("Charge")) {
						//Charge
						FunctionsLua::CastSpellByName("Charge");
					}
					else if (!hasTargetAggro && !targetPlayer && FunctionsLua::IsSpellReady("Mocking Blow")) {
						//Mocking Blow
						FunctionsLua::CastSpellByName("Mocking Blow");
					}
					else if ((localPlayer->rage < 25) && Combat && FunctionsLua::IsSpellReady("Bloodrage")) {
						//Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Shield Bash")) {
						//Shield Bash (Caster)
						FunctionsLua::CastSpellByName("Shield Bash");
					}
					else if (targetPlayer && !HamstringDebuff && FunctionsLua::IsSpellReady("Hamstring")) {
						//Hamstring (PvP)
						FunctionsLua::CastSpellByName("Hamstring");
					}
					else if ((nbrCloseEnemy >= 3) && !ThunderClapDebuff && FunctionsLua::IsSpellReady("Thunder Clap")) {
						//Thunder Clap
						FunctionsLua::CastSpellByName("Thunder Clap");
					}
					else if (FunctionsLua::IsSpellReady("Execute")) {
						//Execute
						FunctionsLua::CastSpellByName("Execute");
					}
					else if (FunctionsLua::IsSpellReady("Overpower")) {
						//Overpower
						FunctionsLua::CastSpellByName("Overpower");
					}
					else if (!RendDebuff && (targetUnit->creatureType != Undead) && (targetUnit->creatureType != Mechanical) && FunctionsLua::IsSpellReady("Rend") && FunctionsLua::UnitIsElite("target")) {
						//Rend
						FunctionsLua::CastSpellByName("Rend");
					}
					else if (FunctionsLua::IsSpellReady("Heroic Strike") && distTarget < 5.0f) {
						//Heroic Strike
						FunctionsLua::CastSpellByName("Heroic Strike");
					}
					else if (FunctionsLua::IsSpellReady("Sunder Armor") && FunctionsLua::UnitIsElite("target")) {
						//Sunder Armor
						FunctionsLua::CastSpellByName("Sunder Armor");
					}
					else if(Combat && localPlayer->rage < 5) {
						//Defensive Stance
						FunctionsLua::CastSpellByName("Defensive Stance");
					}
				}
				else if (DefensiveStance) {
					int ShieldBlockIDs[1] = { 2565 }; bool ShieldBlockBuff = localPlayer->hasBuff(ShieldBlockIDs, 1);
					int nbrAggroParty = 0; for (int i = 1; i <= NumGroupMembers; i++) { nbrAggroParty += HasAggro[i].size(); }
					int RendIDs[7] = { 772, 6546, 6547, 6548, 11572, 11573, 11574 };
					bool RendDebuff = targetUnit->hasDebuff(RendIDs, 7);
					if (!Combat && (distTarget > 8.0f) && (FunctionsLua::GetSpellCooldownDuration("Charge") < 1.0f)) {
						//Battle Stance
						FunctionsLua::CastSpellByName("Battle Stance");
					}
					else if ((nbrAggroParty >= 4) && FunctionsLua::IsSpellReady("Challenging Shout")) {
						//Challenging Shout
						FunctionsLua::CastSpellByName("Challenging Shout");
					}
					else if (!hasTargetAggro && !targetPlayer && FunctionsLua::IsSpellReady("Taunt")) {
						//Taunt
						FunctionsLua::CastSpellByName("Taunt");
					}
					else if ((localPlayer->rage < 25) && Combat && FunctionsLua::IsSpellReady("Bloodrage")) {
						//Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Shield Bash")) {
						//Shield Bash (Caster)
						FunctionsLua::CastSpellByName("Shield Bash");
					}
					else if ((nbrCloseEnemyFacing >= 1) && !ShieldBlockBuff && FunctionsLua::IsSpellReady("Shield Block")) {
						//Shield Block
						FunctionsLua::CastSpellByName("Shield Block");
					}
					else if (FunctionsLua::IsSpellReady("Revenge")) {
						//Revenge
						FunctionsLua::CastSpellByName("Revenge");
					}
					else if (!targetStunned && !targetConfused && FunctionsLua::IsSpellReady("Concussion Blow")) {
						//Concussion Blow
						FunctionsLua::CastSpellByName("Concussion Blow");
					}
					else if (FunctionsLua::IsSpellReady("Shield Slam")) {
						//Shield Slam
						FunctionsLua::CastSpellByName("Shield Slam");
					}
					else if ((localPlayer->rage >= 20) && (nbrCloseEnemyFacing >= 2) && FunctionsLua::IsSpellReady("Cleave")) {
						//Cleave (dump excessive rage)
						FunctionsLua::CastSpellByName("Cleave");
					}
					else if (!RendDebuff && (targetUnit->creatureType != Undead) && (targetUnit->creatureType != Mechanical) && FunctionsLua::IsSpellReady("Rend") && FunctionsLua::UnitIsElite("target")) {
						//Rend
						FunctionsLua::CastSpellByName("Rend");
					}
					else if (FunctionsLua::IsSpellReady("Sunder Armor") && FunctionsLua::UnitIsElite("target")) {
						//Sunder Armor (threat generator)
						FunctionsLua::CastSpellByName("Sunder Armor");
					}
					else if ((localPlayer->rage >= 20) && FunctionsLua::IsSpellReady("Heroic Strike")) {
						//Heroic Strike (dump excessive rage)
						FunctionsLua::CastSpellByName("Heroic Strike");
					}
				}
				else if (BerserkerStance) {
					int HamstringIDs[3] = { 1715, 7372, 7373 };
					bool HamstringDebuff = targetUnit->hasDebuff(HamstringIDs, 3);
					if ((distTarget < 25.0f) && FunctionsLua::IsSpellReady("Intercept")) {
						//Intercept
						FunctionsLua::CastSpellByName("Intercept");
					}
					else if ((localPlayer->rage < 25) && Combat && FunctionsLua::IsSpellReady("Bloodrage")) {
						//Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (FunctionsLua::IsSpellReady("Berserker Rage")) {
						//Berserker Rage
						FunctionsLua::CastSpellByName("Berserker Rage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && FunctionsLua::IsSpellReady("Pummel")) {
						//Pummel (Caster)
						FunctionsLua::CastSpellByName("Pummel");
					}
					else if (targetPlayer && !HamstringDebuff && FunctionsLua::IsSpellReady("Hamstring")) {
						//Hamstring (PvP)
						FunctionsLua::CastSpellByName("Hamstring");
					}
					else if ((nbrCloseEnemy >= 3) && FunctionsLua::IsSpellReady("Whirlwind")) {
						//Whirlwind
						FunctionsLua::CastSpellByName("Whirlwind");
					}
					else if (FunctionsLua::IsSpellReady("Execute")) {
						//Execute
						FunctionsLua::CastSpellByName("Execute");
					}
					else if(Combat && localPlayer->rage < 5) {
						//Defensive Stance
						FunctionsLua::CastSpellByName("Defensive Stance");
					}
				}
			}
		});
	}
}