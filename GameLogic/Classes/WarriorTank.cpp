#include "../ListAI.h"
#include "../MemoryManager.h"
#include <iostream>

void ListAI::WarriorTank() {
	ThreadSynchronizer::RunOnMainThread([=]() {
		int HeroicStrikeIDs[] = { 78, 284, 285, 1608, 11564, 11565, 11566, 11567, 25286 };
		int InvincibleBuffs[] = { 498, 5573, 642, 1020, 1022, 5599, 10278 };
		int InvincibleBuffID = localPlayer->hasBuff(InvincibleBuffs, 7);
		if (Combat && localPlayer->prctHP > 80 && InvincibleBuffID) {
			// Cancel Invincibility
			Functions::CancelPlayerBuff(InvincibleBuffID);
		}
		else if (Combat && (localPlayer->prctHP < 40) && (FunctionsLua::GetHealthstoneCD() < 1.25)) {
			// Healthstone
			FunctionsLua::UseHealthstone();
		}
		else if (Combat && (localPlayer->prctHP < 35) && (FunctionsLua::GetHPotionCD() < 1.25)) {
			// Healing Potion
			FunctionsLua::UseHPotion();
		}
		else if (Combat && (localPlayer->prctHP < 40) && Functions::IsSpellReady("Last Stand")) {
			// Last Stand
			FunctionsLua::CastSpellByName("Last Stand");
		}
		else if (Combat && (localPlayer->prctHP < 25) && Functions::IsSpellReady("Shield Wall")) {
			// Shield Wall
			FunctionsLua::CastSpellByName("Shield Wall");
		}
		else if ((localPlayer->castInfo == 0 || localPlayer->isCasting(HeroicStrikeIDs, 9)) && localPlayer->channelInfo == 0 && !localPlayer->isdead && !passiveGroup) {
			int BattleShoutIDs[7] = { 6673, 5242, 6192, 11549, 11550, 11551, 25289 }; bool BattleShoutBuff = localPlayer->hasBuff(BattleShoutIDs, 7);
			if (ListAI::TankTargeting()) {}
			else if ((nbrCloseEnemy >= 4) && Functions::IsSpellReady("Intimidating Shout")) {
				// Intimidating Shout
				FunctionsLua::CastSpellByName("Intimidating Shout");
			}
			else if (Combat && !BattleShoutBuff && Functions::IsSpellReady("Battle Shout")) {
				// Battle Shout
				FunctionsLua::CastSpellByName("Battle Shout");
			}
			else if (targetUnit != NULL && targetUnit->attackable && !targetUnit->isdead) {
				bool targetPlayer = targetUnit->flags & UNIT_FLAG_PLAYER_CONTROLLED;
				bool targetStunned = targetUnit->flags & UNIT_FLAG_STUNNED;
				bool targetConfused = targetUnit->flags & UNIT_FLAG_CONFUSED;
				bool BattleStance = FunctionsLua::GetShapeshiftFormInfo(1);
				bool DefensiveStance = FunctionsLua::GetShapeshiftFormInfo(2);
				bool BerserkerStance = FunctionsLua::GetShapeshiftFormInfo(3);
				int DemoralizingShoutIDs[5] = { 1160, 6190, 11554, 11555, 11556 }; bool DemoralizingShoutDebuff = targetUnit->hasDebuff(DemoralizingShoutIDs, 5);
				if (!Functions::IsCurrentAction("Attack")) Functions::InteractUnit(targetUnit->Pointer, 1);
				if ((nbrCloseEnemy >= 3) && !DemoralizingShoutDebuff && Functions::IsSpellReady("Demoralizing Shout")) {
					// Demoralizing Shout
					FunctionsLua::CastSpellByName("Demoralizing Shout");
				}
				else if (BattleStance) {
					int HamstringIDs[3] = { 1715, 7372, 7373 };
					bool HamstringDebuff = targetUnit->hasDebuff(HamstringIDs, 3);
					int ThunderClapIDs[5] = { 6343, 8198, 8205, 11580, 11581 };
					bool ThunderClapDebuff = targetUnit->hasDebuff(ThunderClapIDs, 5);
					int RendIDs[7] = { 772, 6546, 6547, 6548, 11572, 11573, 11574 };
					bool RendDebuff = targetUnit->hasDebuff(RendIDs, 7);
					if ((distTarget < 25.0f) && Functions::IsSpellReady("Charge")) {
						// Charge
						FunctionsLua::CastSpellByName("Charge");
					}
					else if (!hasTargetAggro && !targetPlayer && Functions::IsSpellReady("Mocking Blow")) {
						// Mocking Blow
						FunctionsLua::CastSpellByName("Mocking Blow");
					}
					else if ((localPlayer->rage < 25) && Combat && Functions::IsSpellReady("Bloodrage")) {
						// Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && Functions::IsSpellReady("Shield Bash")) {
						// Shield Bash (Caster)
						FunctionsLua::CastSpellByName("Shield Bash");
					}
					else if (targetPlayer && !HamstringDebuff && Functions::IsSpellReady("Hamstring")) {
						// Hamstring (PvP)
						FunctionsLua::CastSpellByName("Hamstring");
					}
					else if ((nbrCloseEnemy >= 3) && !ThunderClapDebuff && Functions::IsSpellReady("Thunder Clap")) {
						// Thunder Clap
						FunctionsLua::CastSpellByName("Thunder Clap");
					}
					else if (Functions::IsSpellReady("Execute")) {
						// Execute
						FunctionsLua::CastSpellByName("Execute");
					}
					else if (Functions::IsSpellReady("Overpower")) {
						// Overpower
						FunctionsLua::CastSpellByName("Overpower");
					}
					else if (targetPlayer && !RendDebuff && (FunctionsLua::UnitClass("target") == "Rogue" || FunctionsLua::UnitClass("target") == "Druid") && Functions::IsSpellReady("Rend")) {
						// Rend
						FunctionsLua::CastSpellByName("Rend");
					}
					else if (Functions::IsSpellReady("Heroic Strike") && distTarget < 5.0f) {
						// Heroic Strike
						FunctionsLua::CastSpellByName("Heroic Strike");
					}
					else if (Functions::IsSpellReady("Sunder Armor") && FunctionsLua::UnitIsElite("target")) {
						// Sunder Armor
						FunctionsLua::CastSpellByName("Sunder Armor");
					}
					else if(Combat && localPlayer->rage < 5) {
						// Defensive Stance
						FunctionsLua::CastSpellByName("Defensive Stance");
					}
				}
				else if (DefensiveStance) {
					int ShieldBlockIDs[1] = { 2565 }; bool ShieldBlockBuff = localPlayer->hasBuff(ShieldBlockIDs, 1);
					int nbrAggroParty = 0; for (int i = 1; i <= NumGroupMembers; i++) { nbrAggroParty += HasAggro[i].size(); }
					int RendIDs[7] = { 772, 6546, 6547, 6548, 11572, 11573, 11574 };
					bool RendDebuff = targetUnit->hasDebuff(RendIDs, 7);
					if (!Combat && (distTarget > 10.0f) && (FunctionsLua::GetSpellCooldownDuration("Charge") < 1.0f)) {
						// Battle Stance
						FunctionsLua::CastSpellByName("Battle Stance");
					}
					else if ((nbrAggroParty >= 4) && Functions::IsSpellReady("Challenging Shout")) {
						// Challenging Shout
						FunctionsLua::CastSpellByName("Challenging Shout");
					}
					else if (!hasTargetAggro && !targetPlayer && targetUnit->targetGuid != 0 && Functions::IsSpellReady("Taunt")) {
						// Taunt
						FunctionsLua::CastSpellByName("Taunt");
					}
					else if ((localPlayer->rage < 25) && Combat && Functions::IsSpellReady("Bloodrage")) {
						// Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && Functions::IsSpellReady("Shield Bash")) {
						// Shield Bash (Caster)
						FunctionsLua::CastSpellByName("Shield Bash");
					}
					else if ((nbrCloseEnemyFacing >= 1) && hasTargetAggro && !ShieldBlockBuff && !FunctionsLua::UnitIsCaster("target") && Functions::IsSpellReady("Shield Block")) {
						// Shield Block
						FunctionsLua::CastSpellByName("Shield Block");
					}
					else if (Functions::IsSpellReady("Revenge")) {
						// Revenge
						FunctionsLua::CastSpellByName("Revenge");
					}
					else if (!targetStunned && !targetConfused && Functions::IsSpellReady("Concussion Blow")) {
						// Concussion Blow
						FunctionsLua::CastSpellByName("Concussion Blow");
					}
					else if (Functions::IsSpellReady("Shield Slam")) {
						// Shield Slam
						FunctionsLua::CastSpellByName("Shield Slam");
					}
					else if ((localPlayer->rage >= 20) && (nbrCloseEnemyFacing >= 2) && Functions::IsSpellReady("Cleave")) {
						// Cleave (dump excessive rage)
						FunctionsLua::CastSpellByName("Cleave");
					}
					else if (targetPlayer && !RendDebuff && (FunctionsLua::UnitClass("target") == "Rogue" || FunctionsLua::UnitClass("target") == "Druid") && Functions::IsSpellReady("Rend")) {
						// Rend
						FunctionsLua::CastSpellByName("Rend");
					}
					else if (Functions::IsSpellReady("Sunder Armor") && FunctionsLua::UnitIsElite("target")) {
						// Sunder Armor (threat generator)
						FunctionsLua::CastSpellByName("Sunder Armor");
					}
					else if ((localPlayer->rage >= 20) && Functions::IsSpellReady("Heroic Strike")) {
						// Heroic Strike (dump excessive rage)
						FunctionsLua::CastSpellByName("Heroic Strike");
					}
				}
				else if (BerserkerStance) {
					int HamstringIDs[3] = { 1715, 7372, 7373 };
					bool HamstringDebuff = targetUnit->hasDebuff(HamstringIDs, 3);
					if ((distTarget < 25.0f) && Functions::IsSpellReady("Intercept")) {
						// Intercept
						FunctionsLua::CastSpellByName("Intercept");
					}
					else if ((localPlayer->rage < 25) && Combat && Functions::IsSpellReady("Bloodrage")) {
						// Bloodrage
						FunctionsLua::CastSpellByName("Bloodrage");
					}
					else if (Functions::IsSpellReady("Berserker Rage")) {
						// Berserker Rage
						FunctionsLua::CastSpellByName("Berserker Rage");
					}
					else if (IsFacing && !targetStunned && FunctionsLua::UnitIsCaster("target") && Functions::IsSpellReady("Pummel")) {
						// Pummel (Caster)
						FunctionsLua::CastSpellByName("Pummel");
					}
					else if (targetPlayer && !HamstringDebuff && Functions::IsSpellReady("Hamstring")) {
						// Hamstring (PvP)
						FunctionsLua::CastSpellByName("Hamstring");
					}
					else if ((nbrCloseEnemy >= 3) && Functions::IsSpellReady("Whirlwind")) {
						// Whirlwind
						FunctionsLua::CastSpellByName("Whirlwind");
					}
					else if (Functions::IsSpellReady("Execute")) {
						// Execute
						FunctionsLua::CastSpellByName("Execute");
					}
					else if(Combat && localPlayer->rage < 5) {
						// Defensive Stance
						FunctionsLua::CastSpellByName("Defensive Stance");
					}
				}
			}
		}
	});
}
