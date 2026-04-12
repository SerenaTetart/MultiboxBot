#pragma once
#include "Functions.h"

static int GetIntFromChar(const char* txt) {
	//Obtient le !premier! nombre d'un char*
	int length = strlen(txt);
	int ret = 0; int y = 0; int lenNbr = 0;
	for (int i = 0; i < length; i++) {
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') lenNbr++;
		else if (lenNbr > 0) break;
	}
	for (int i = 0; i < length; i++) {
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') {
			ret = ret + int(powf(10, lenNbr - y - 1) * ((int)txt[i] - '0'));
			y++;
		}
		else if (y > 0) break;
	}
	return ret;
}

static float GetFloatFromChar(const char* txt) {
	int length = strlen(txt); int y = 0;
	float ret = 0; int indDecimal = 0;
	for (int i = 0; i < length; i++) {
		if (txt[i] == ',' || txt[i] == '.') indDecimal = i;
	}
	for (int i = 0; i < length; i++) {
		if ((int)txt[i] >= '0' && (int)txt[i] <= '9') {
			if(indDecimal > 0)
				ret = ret + (powf(10, length - (length - indDecimal) - y - 1) * (float)((int)txt[i] - '0'));
			else
				ret = ret + (powf(10, length - y - 1) * (float)((int)txt[i] - '0'));
			y++;
		}
	}
	return ret;
}

class FunctionsLua {
    public:
        // === Group Functions === //
    static int GetNumGroupMembers();
    static bool HasPetUI();
    static int GetPetHappiness();
    static WoWUnit* GetHealer();
	static bool IsInInstance();
        // === Timer/CD === //
	static float GetTime();
	static float GetItemCooldownDuration(int item_id);
	static float GetItemCooldownDuration(int* items_id, int size);
	static float GetActionCooldownDuration(int slot);
	static float GetSpellCooldownDuration(std::string spell_name);
        // === Merchant/Trading skills === //
	static int GetMerchantNumItems();
	static int GetRepairAllCost();
	static int GetTradingSkill(std::string name);
	static std::tuple<int, int> GetTradeSkillList(std::string names[], int size);
        static void SellUselessItems();
        // === Items === //
	static int GetContainerNumSlots(int slot);
	static std::tuple<std::string, int> GetContainerItemInfo(int bag, int slot);
	static std::string GetContainerItemLink(int bag, int slot);
	static bool IsInventoryFull();
	static int GetItemCount(int item_info);
	static bool HasItem(int* item_id, int size);
	static bool PickupItem(int item_id);
	static void PickupItem(int x, int y);
	static void MakeVirtualInventory(std::vector<std::tuple<int, int, int, std::string>> *listItems);
	static void DropItemOnUnit(std::string target);
	static void UseItem(int item_id);
	static void UseHPotion();
	static void UseMPotion();
	static void UseHealthstone();
	static int HasDrink(); static int HasMeat();
	static bool HasHPotion(); static bool HasMPotion(); static bool HasHealthstone();
	static float GetHPotionCD(); static float GetMPotionCD(); static float GetHealthstoneCD();
	static int GetItemQuality(std::string item_link);
        // === Buffs / Debuffs === //
	static std::string UnitBuff(std::string target, int index);
	static std::tuple<std::string, int, std::string> UnitDebuff(std::string target, int index);
	static bool GetUnitBuff(std::string target, std::string texture);
	static bool GetUnitDebuff(std::string target, std::string texture);
	static int GetStackDebuff(std::string target, std::string texture);
	static bool GetUnitDispel(std::string target, std::string dispellType1, std::string dispellType2="Null", std::string dispellType3="Null");
	static WoWUnit* GetGroupDispel(std::string dispellType1, std::string dispellType2="Null", std::string dispellType3="Null");
        // === Spells/Actions === //
	static bool GetShapeshiftFormInfo(int nbr);
	static int GetNumSpellTabs();
	static std::string GetSpellName(int id);
	static std::string GetSpellTexture(int spellID);
	static std::tuple<std::string, std::string, int, int> GetSpellTabInfo(int index);
	static std::tuple<int, int> GetSpellID(std::string spell_name, bool spell_exist=false);
	static bool IsPlayerSpell(std::string spell_name);
	static bool IsSpellReady(std::string spell_name);
	static void CastSpellByName(std::string spell_name);
	static void UseAction(int slot, int self=0);
	static bool IsAutoRepeatAction(int slot);
	static bool IsUsableAction(int slot);
	static bool HasAction(int slot);
	static std::string GetActionTexture(int slot);
	static bool IsConsumableAction(int slot);
	static bool IsActionInRange(int slot);
	static bool SpellIsTargeting();
	static void SpellStopTargeting();
	static int GetSlot(std::string spell_name, std::string slot_type="SPELL");
	// === Unit === //
	static int UnitStat(std::string target, int nbr);
	static void TargetUnit(std::string target);
	static std::string UnitName(std::string target);
	static bool UnitCanAttack(std::string char1, std::string char2);
	static bool UnitIsDeadOrGhost(std::string char1);
	static bool CheckInteractDistance(std::string char1, int dist);
	static bool UnitAffectingCombat(std::string target);
	static std::string UnitClass(std::string target);
	static bool UnitIsCaster(std::string target);
	static bool UnitIsElite(std::string target);
	static std::string UnitCreatureType(std::string target);
	static bool IsGroupInCombat();
	static bool IsShieldEquipped();
	static bool HasWandEquipped();
	static int GetComboPoints();
	static int GetTalentInfo(int page, int index);
	static bool IsCurrentAction(int slot);
	static float UnitAttackSpeed(std::string target);
	static void FollowUnit(std::string target);
};
