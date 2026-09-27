#include "FunctionsLua.h"
#include "Game.h"
#include <iostream>

//======================================================================//
//========================   Group Functions   =========================//
//======================================================================//

int FunctionsLua::GetNumGroupMembers() {
	Functions::LuaCall("count = GetNumRaidMembers()");
	int nbrRaid = GetIntFromChar((char*)Functions::GetText("count"));
	Functions::LuaCall("count = GetNumPartyMembers()");
	int nbrParty = GetIntFromChar((char*)Functions::GetText("count"));
	if (nbrRaid > nbrParty) return nbrRaid;
	else return nbrParty;
}

bool FunctionsLua::HasPetUI() {
	Functions::LuaCall("UI = HasPetUI()");
	int hasUI = GetIntFromChar((char*)Functions::GetText("UI"));
	if (hasUI == 1) return true;
	else return false;
}

int FunctionsLua::GetPetHappiness() {
	Functions::LuaCall("hap = GetPetHappiness()");
	int hapiness = GetIntFromChar((char*)Functions::GetText("hap"));
	return hapiness;
}

WoWUnit* FunctionsLua::GetHealer() {
	for (int i = 1; i <= NumGroupMembers; i++) {
		if (GroupMember[i] == NULL) continue;
		std::string grClass = UnitClass(tarType+std::to_string(i));
		if (grClass == "Priest" || grClass == "Paladin" || grClass == "Shaman") return GroupMember[i];
	}
	return NULL;
}

bool FunctionsLua::IsInInstance() {
	Functions::LuaCall("inInstance = IsInInstance()");
	int inInstance = GetIntFromChar((char*)Functions::GetText("inInstance"));
	if (inInstance == 1) return true;
	else return false;
}

//======================================================================//
//============================   Timer/CD   ============================//
//======================================================================//

float FunctionsLua::GetTime() {
	Functions::LuaCall("time = GetTime()");
	float time = GetFloatFromChar((char*)Functions::GetText("time"));
	return time;
}

float FunctionsLua::GetItemCooldownDuration(int item_id) {
	for (const auto& item : virtualInventory) {
		if (item.id == item_id) {
			std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(item.bag) + +", " + std::to_string(item.slot) + ")";
			Functions::LuaCall(command.c_str());
			float start = GetFloatFromChar((char*)Functions::GetText("start"));
			float duration = GetFloatFromChar((char*)Functions::GetText("duration"));
			float cdLeft = start + duration - GetTime();
			if (cdLeft < 0) cdLeft = 0;
			return cdLeft;
		}
	}
	return 999;
}

float FunctionsLua::GetItemCooldownDuration(int* items_id, int size) {
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < size; i++) {
			if (item.id == items_id[i]) {
				std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(item.bag) + +", " + std::to_string(item.slot) + ")";
				Functions::LuaCall(command.c_str());
				float start = GetFloatFromChar((char*)Functions::GetText("start"));
				float duration = GetFloatFromChar((char*)Functions::GetText("duration"));
				float cdLeft = start + duration - GetTime();
				if (cdLeft < 0) cdLeft = 0;
				return cdLeft;
			}
		}
	}
	return 999;
}

float FunctionsLua::GetActionCooldownDuration(int slot) {
	std::string command = "start, duration = GetActionCooldown(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	float start = GetFloatFromChar((char*)Functions::GetText("start"));
	float duration = GetFloatFromChar((char*)Functions::GetText("duration"));
	float cdLeft = start + duration - GetTime();
	if (cdLeft < 0) cdLeft = 0;
	return cdLeft;
}

float FunctionsLua::GetSpellCooldownDuration(std::string spell_name) {
	SpellSlotData spell = GetSpellData(spell_name);
	if (spell.id > 0) {
		std::string command = "start, duration = GetSpellCooldown(" + std::to_string(spell.slot) + ", BOOKTYPE_SPELL)";
		Functions::LuaCall(command.c_str());
		float start = GetFloatFromChar((char*)Functions::GetText("start"));
		float duration = GetFloatFromChar((char*)Functions::GetText("duration"));
		float cdLeft = start + duration - GetTime();
		if (cdLeft < 0) cdLeft = 0;
		return cdLeft;
	} else return 999;
}

//======================================================================//
//========================   Merchant/Trading   ========================//
//======================================================================//

int FunctionsLua::GetMerchantNumItems() {
	Functions::LuaCall("count = GetMerchantNumItems()");
	int nbr = GetIntFromChar((char*)Functions::GetText("count"));
	return nbr;
}

int FunctionsLua::GetRepairAllCost() {
	Functions::LuaCall("cost = GetRepairAllCost()");
	int cost = GetIntFromChar((char*)Functions::GetText("cost"));
	return cost;
}

void FunctionsLua::SellUselessItems() {
	int craftItems[] = {2901 ,5956, 7005, };
	for (const auto& item : virtualInventory) {
		bool craftItem = false;
		for (int i = 0; i < 3; i++) {
			if (item.id == craftItems[i]) {
				craftItem = true;
				break;
			}
		}
		if (!craftItem && (item.quality == 0 || ((item.type == "Weapon" || item.type == "Armor") && item.quality <= 3 && item.minLevel < localPlayer->level))) {
			// Sell grey items OR equipment under epic and under player's level
			std::string command = "UseContainerItem(" + std::to_string(item.bag) + ", " + std::to_string(item.slot) + ")";
			Functions::LuaCall(command.c_str());
			return;
		}
	}
}

std::string FunctionsLua::GetTradePlayerItemLink(int id) {
	std::string command = "itemLink = GetTradePlayerItemLink(" + std::to_string(id) + ")";
	Functions::LuaCall(command.c_str());
	std::string itemLink = (char*)Functions::GetText("itemLink");
	return itemLink;
}

std::string FunctionsLua::GetTradeTargetItemLink(int id) {
	std::string command = "itemLink = GetTradeTargetItemLink(" + std::to_string(id) + ")";
	Functions::LuaCall(command.c_str());
	std::string itemLink = (char*)Functions::GetText("itemLink");
	return itemLink;
}

int FunctionsLua::GetTradingSkill(std::string name) {
	Functions::LuaCall("numSkill = GetNumSkillLines()");
	int numSkill = GetIntFromChar((char*)Functions::GetText("numSkill"));
	for (int i = 0; i < numSkill; i++) {
		std::string luaCommand = "n,h,_,r,_,_,m=GetSkillLineInfo(" + std::to_string(i) + ")";
		Functions::LuaCall(luaCommand.c_str());
		bool h = (GetIntFromChar((char*)Functions::GetText("h")) != false);
		if (h) continue;
		//int m = GetIntFromChar((char*)Functions::GetText("m"));
		std::string n = (char*)Functions::GetText("n");
		if (!h && name == n) return GetIntFromChar((char*)Functions::GetText("r"));
	}
	return -1;
}

std::tuple<int, int> FunctionsLua::GetTradeSkillList(std::string names[], int size) {
	Functions::LuaCall("numSkill = GetNumSkillLines()");
	int numSkill = GetIntFromChar((char*)Functions::GetText("numSkill"));
	int skills[] = { 0, 0 }; int z = 0;
	for (int i = 0; i < numSkill; i++) {
		std::string luaCommand = "n,h,_,r,_,_,m=GetSkillLineInfo(" + std::to_string(i) + ")";
		Functions::LuaCall(luaCommand.c_str());
		bool h = (GetIntFromChar((char*)Functions::GetText("h")) != false);
		if (h) continue;
		std::string n = (char*)Functions::GetText("n");
		for (int y = 0; y < size; y++) {
			if (z > 2) break;
			else if (!h && names[y] == n) {
				skills[z] = y+1;
				z++;
				break;
			}
		}
		if (z > 2) break;
	}
	return std::make_tuple(skills[0], skills[1]);
}

//======================================================================//
//=============================   Items   ==============================//
//======================================================================//

int FunctionsLua::GetItemQuality(const std::string& item_link) {
	if (item_link.size() < 10) return -1;
	std::string_view color(item_link.data() + 4, 6);

	if (color == "9d9d9d") return 0; // Poor
	if (color == "ffffff") return 1; // Common
	if (color == "1eff00") return 2; // Uncommon
	if (color == "0070dd") return 3; // Rare
	if (color == "a335ee") return 4; // Epic
	if (color == "ff8000") return 5; // Legendary
	if (color == "e6cc80") return 6; // Artifact

	return -1;
}

void FunctionsLua::MakeVirtualInventory(std::vector<InventoryItem>* listItems) {
	listItems->clear();
	inventoryFull = true;

	int item_count, item_quality, item_min_level;
	bool item_locked, item_readable, item_lootable;
	std::string item_texture, item_link, item_type, item_subtype;

	for (int bag = 0; bag <= 4; ++bag)
	{
		for (int slot = 1; slot <= GetContainerNumSlots(bag); ++slot)
		{
			std::string item_link = GetContainerItemLink(bag, slot);
			if (item_link.empty() || item_link.size() <= 11) {
				inventoryFull = false;
				continue;
			}

			std::tie(
				item_texture,
				item_count,
				item_locked,
				item_readable,
				item_lootable
			) = GetContainerItemInfo(bag, slot);

			int item_id = GetIntFromChar(item_link.c_str() + 11);
			int item_quality = GetItemQuality(item_link);

			std::tie(
				item_min_level,
				item_type,
				item_subtype
			) = GetItemInfo(item_id);

			listItems->push_back({
				bag,
				slot,
				item_id,
				item_texture,
				item_count,
				item_locked,
				item_quality,
				item_readable,
				item_lootable,
				item_min_level,
				item_type,
				item_subtype
			});
		}
	}
}

int FunctionsLua::GetContainerNumSlots(int slot) {
	std::string command = "slots = GetContainerNumSlots(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int slots = GetIntFromChar((char*)Functions::GetText("slots"));
	return slots;
}

std::string FunctionsLua::GetContainerItemLink(int bag, int slot) {
	std::string command = "link = GetContainerItemLink(" + std::to_string(bag) + ", " + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	std::string link = (char*)Functions::GetText("link");
	return link;
}

std::tuple<std::string, int, bool, bool, bool> FunctionsLua::GetContainerItemInfo(int bag, int slot) {
	std::string command = "texture, itemCount, locked, _, readable, lootable = GetContainerItemInfo("+std::to_string(bag)+", "+std::to_string(slot)+")";
	Functions::LuaCall(command.c_str());
	std::string texture = (char*)Functions::GetText("texture");
	int itemCount = GetIntFromChar((char*)Functions::GetText("itemCount"));
	bool locked = (GetIntFromChar((char*)Functions::GetText("locked")) == 1);
	bool readable = (GetIntFromChar((char*)Functions::GetText("readable")) == 1);
	bool lootable = (GetIntFromChar((char*)Functions::GetText("lootable")) == 1);
	return std::make_tuple(texture, itemCount, locked, readable, lootable);
}

std::tuple<int, std::string, std::string> FunctionsLua::GetItemInfo(int item_id) {
	std::string command = "_, _, _, itemMinLevel, itemType, itemSubtype = GetItemInfo("+std::to_string(item_id)+")";
	Functions::LuaCall(command.c_str());
	int itemMinLevel = GetIntFromChar((char*)Functions::GetText("itemMinLevel"));
	std::string itemType = (char*)Functions::GetText("itemType");
	std::string itemSubtype = (char*)Functions::GetText("itemSubtype");
	return std::make_tuple(itemMinLevel, itemType, itemSubtype);
}

void FunctionsLua::CloseLoot() {
	std::string command = "CloseLoot(0)";
	Functions::LuaCall(command.c_str());
}

int FunctionsLua::GetItemCount(int item_id) {
	int total = 0;
	for (const auto& item : virtualInventory) {
		if (item.id == item_id) {
			int itemCount = item.count;
			total = total + itemCount;
		}
	}
	return total;
}

bool FunctionsLua::HasItem(int* item_ids, int size) {
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < size; i++) {
			if (item.id == item_ids[i]) return true;
		}
	}
	return false;
}

bool FunctionsLua::PickupItem(int item_id) {
	for (const auto& item : virtualInventory) {
		if (item.id == item_id && !item.locked) {
			std::string command = "PickupContainerItem(" + std::to_string(item.bag) + ", " + std::to_string(item.slot) + ")";
			Functions::LuaCall(command.c_str());
			return true;
		}
	}
	return false;
}

void FunctionsLua::PickupItem(int x, int y) {
	std::string command = "PickupContainerItem(" + std::to_string(x) + ", " + std::to_string(y) + ")";
	Functions::LuaCall(command.c_str());
}

void FunctionsLua::DropItemOnUnit(std::string target) {
	std::string command = "DropItemOnUnit(\"" + target + "\")";
	Functions::LuaCall(command.c_str());
}

void FunctionsLua::UseItem(int item_id) {
	//Use the indicated item
	for (const auto& item : virtualInventory) {
		if (item.id == item_id && !item.locked) {
			std::string command = "UseContainerItem(" + std::to_string(item.bag) + ", " + std::to_string(item.slot) + ")";
			Functions::LuaCall(command.c_str());
			return;
		}
	}
}

int FunctionsLua::HasDrink() {
	int listID[20] = { 159, 1179, 1205, 1645, 1708, 2136, 2288, 3772, 4791, 5350, 8077
		, 8078, 8079, 8766, 9451, 10841, 13724, 18300, 19301, 20031 };
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 20; i++) {
			if (item.id == listID[i]) return item.id;
		}
	}
	return 0;
}

int FunctionsLua::HasMeat() {
	int CookedlistID[47] = { 117, 724, 1017, 2287, 2679, 2680, 2681, 2684, 2685, 2687, 2888, 3220, 3662, 3664, 3726, 3727, 3728, 3770, 3771, 4457, 4599, 5472, 5474, 5477, 5478, 5479, 7097, 8952, 11444, 12209, 12210, 12211, 12213, 12215, 12216, 12224, 13851, 17119, 17222, 17407, 17408, 18045, 19224, 19304, 19305, 20074, 21023 };
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 47; i++) {
			if (item.id == CookedlistID[i]) return item.id;
		}
	}
	return 0;
}

void FunctionsLua::UseHPotion() {
	int listID[6] = { 118, 858, 929, 1710, 3928, 13446 };
	for (int i = 0; i < 6; i++) {
		UseItem(listID[i]);
	}
}

bool FunctionsLua::HasHPotion() {
	int listID[6] = { 118, 858, 929, 1710, 3928, 13446};
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 6; i++) {
			if (item.id == listID[i]) return true;
		}
	}
	return false;
}

float FunctionsLua::GetHPotionCD() {
	int listID[6] = { 118, 858, 929, 1710, 3928, 13446 };
	float CD = GetItemCooldownDuration(listID, 6);
	return CD;
}

bool FunctionsLua::HasMPotion() {
	int listID[6] = { 2455, 3385, 3827, 6149, 13443, 13444 };
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 6; i++) {
			if (item.id == listID[i]) return true;
		}
	}
	return false;
}

void FunctionsLua::UseMPotion() {
	int listID[6] = { 2455, 3385, 3827, 6149, 13443, 13444 };
	for (int i = 0; i < 6; i++) {
		UseItem(listID[i]);
	}
}

float FunctionsLua::GetMPotionCD() {
	int listID[6] = { 2455, 3385, 3827, 6149, 13443, 13444 };
	float CD = GetItemCooldownDuration(listID, 6);
	return CD;
}

bool FunctionsLua::HasHealthstone() {
	int listID[] = { 5512, 19004, 19005, 5511, 19006, 19007, 5509, 19008, 19009, 5510, 19010, 19011, 9421, 19012, 19013 };
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 15; i++) {
			if (item.id == listID[i]) return true;
		}
	}
	return false;
}

void FunctionsLua::UseHealthstone() {
	int listID[] = { 5512, 19004, 19005, 5511, 19006, 19007, 5509, 19008, 19009, 5510, 19010, 19011, 9421, 19012, 19013 };
	for (int i = 0; i < 15; i++) {
		UseItem(listID[i]);
	}
}

float FunctionsLua::GetHealthstoneCD() {
	int listID[15] = { 5512, 19004, 19005, 5511, 19006, 19007, 5509, 19008, 19009, 5510, 19010, 19011, 9421, 19012, 19013 };
	float CD = GetItemCooldownDuration(listID, 15);
	return CD;
}

//======================================================================//
//========================   Buffs / Debuffs   =========================//
//======================================================================//

std::string FunctionsLua::UnitBuff(std::string target, int index) {
	std::string command = "texture = UnitBuff(\"" + (std::string)target + "\", " + std::to_string(index) + ")";
	Functions::LuaCall(command.c_str());
	char* texture = (char*)Functions::GetText("texture");
	return texture;
}

std::tuple<std::string, int, std::string> FunctionsLua::UnitDebuff(std::string target, int index) {
	std::string command = "texture,count,debuffType = UnitDebuff(\"" + (std::string)target + "\", " + std::to_string(index) + ")";
	Functions::LuaCall(command.c_str());
	char* texture = (char*)Functions::GetText("texture");
	int count = GetIntFromChar((char*)Functions::GetText("count"));
	char* type = (char*)Functions::GetText("debuffType");
	return std::make_tuple(texture, count, type);
}

bool FunctionsLua::GetUnitBuff(std::string target, std::string texture) {
	for (int i = 1; i <= 30; i++) {
		std::string textname = UnitBuff(target, i);
		if (textname == texture) return true;
		else if (textname == "") return false;
	}
	return false;
}

bool FunctionsLua::GetUnitDebuff(std::string target, std::string texture) {
	for (int i = 1; i <= 16; i++) {
		std::string textname;
		std::tie(textname, std::ignore, std::ignore) = UnitDebuff(target, i);
		if (textname == texture) return true;
		else if (textname == "") return false;
	}
	return false;
}

int FunctionsLua::GetStackDebuff(std::string target, std::string texture) {
	for (int i = 1; i <= 16; i++) {
		int count;
		std::tie(std::ignore, count, std::ignore) = UnitDebuff(target, i);
		return count;
	}
	return 0;
}

bool FunctionsLua::GetUnitDispel(std::string target, std::string dispellType1, std::string dispellType2, std::string dispellType3) {
	//Retourne si la cible a un debuff � dispel
	std::string args[3] = { dispellType1, dispellType2, dispellType3 };
	for (int i = 1; i <= 16; i++) {
		std::string debuffIcon, debuffType;
		std::tie(debuffIcon, std::ignore, debuffType) = UnitDebuff(target, i);
		if (debuffIcon != "Interface\\Icons\\Spell_Frost_FrostArmor02" && debuffIcon != "Interface\\Icons\\Spell_Shadow_Cripple") {
			for (int y = 0; y < 3; y++) {
				if (args[y] == debuffType) return true;
			}
		}
	}
	return false;
}

WoWUnit* FunctionsLua::GetGroupDispel(std::string dispellType1, std::string dispellType2, std::string dispellType3) {
	//Retourne le joueur du groupe � dispel
	for (int i = 1; i <= NumGroupMembers; i++) {
		if ((GroupMember[i] != NULL) && (GroupMember[i]->unitReaction > Neutral)
			&& !GroupMember[i]->isdead && (localPlayer->position.DistanceTo(GroupMember[i]->position) < 40.0f)
			&& !Functions::Intersect(localPlayer->position, GroupMember[i]->position)
			&& GetUnitDispel(tarType + std::to_string(i), dispellType1, dispellType2, dispellType3)) return GroupMember[i];
	}
	return NULL;
}

//======================================================================//
//=========================   Spells/Actions   =========================//
//======================================================================//

bool FunctionsLua::GetShapeshiftFormInfo(int nbr) {
	Functions::LuaCall(("_,_,Stance = GetShapeshiftFormInfo(" + std::to_string(nbr) + ")").c_str());
	int stance = GetIntFromChar((char*)Functions::GetText("Stance"));
	if (stance == 1) return true;
	else return false;
}

int FunctionsLua::GetNumSpellTabs() {
	Functions::LuaCall("res = GetNumSpellTabs()");
	int result = GetIntFromChar((char*)Functions::GetText("res"));
	return result;
}

std::string FunctionsLua::GetSpellName(int id) {
	std::string command = "res = GetSpellName(" + std::to_string(id) + ", BOOKTYPE_SPELL)";
	Functions::LuaCall(command.c_str());
	char* result = (char*)Functions::GetText("res");
	return result;
}

std::string FunctionsLua::GetSpellTexture(int spellID) {
	std::string command = "res = GetSpellTexture(" + std::to_string(spellID) + ", BOOKTYPE_SPELL)";
	Functions::LuaCall(command.c_str());
	std::string result = (char*)Functions::GetText("res");
	return result;
}

std::tuple<std::string, std::string, int, int> FunctionsLua::GetSpellTabInfo(int index) {
	std::string command = "name, texture, offset, numSpells = GetSpellTabInfo(" + std::to_string(index) + ")";
	Functions::LuaCall(command.c_str());
	char* name = (char*)Functions::GetText("name");
	char* texture = (char*)Functions::GetText("texture");
	int offset = GetIntFromChar((char*)Functions::GetText("offset"));
	int numSpells = GetIntFromChar((char*)Functions::GetText("numSpells"));
	return std::make_tuple(name, texture, offset, numSpells);
}

SpellSlotData FunctionsLua::GetSpellData(std::string spell_name) {
	SpellSlotData current_spell;
    for (std::size_t i = 0; i < virtualSpellBook.size(); ++i) {
        if (virtualSpellBook[i].name == spell_name) {
            current_spell = virtualSpellBook[i];
            while (i + 1 < virtualSpellBook.size() && virtualSpellBook[i + 1].name == spell_name) {
                ++i;
                current_spell = virtualSpellBook[i];
            }
            return current_spell;
        }
    }
    return current_spell;
}

bool FunctionsLua::IsPlayerSpell(std::string spell_name) {
	for (const auto& spell : virtualSpellBook) {
		if (spell.name == spell_name) return true;
	}
	return false;
}

void FunctionsLua::CastSpellByName(std::string spell_name) {
	Functions::LuaCall(("CastSpellByName(\"" + spell_name + "\")").c_str());
}

void FunctionsLua::UseAction(int slot, int self) {
	if(self == 1) Functions::LuaCall(("UseAction(" + std::to_string(slot) + ", 0, 1)").c_str());
	else Functions::LuaCall(("UseAction("+std::to_string(slot)+")").c_str());
}

bool FunctionsLua::IsAutoRepeatAction(int slot) {
	Functions::LuaCall(("autoR = IsAutoRepeatAction(" + std::to_string(slot) + ")").c_str());
	int autoR = GetIntFromChar((char*)Functions::GetText("autoR"));
	if (autoR == 1) return true;
	else return false;
}

bool FunctionsLua::IsUsableAction(int slot) {
	std::string command = "usable, nomana = IsUsableAction(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int usable = GetIntFromChar((char*)Functions::GetText("usable"));
	int nomana = GetIntFromChar((char*)Functions::GetText("nomana"));
	if (usable == 1 && nomana == 0) return true;
	else return false;
}

bool FunctionsLua::HasAction(int slot) {
	std::string command = "res = HasAction(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("res"));
	if (result == 1) return true;
	else return false;
}

std::string FunctionsLua::GetActionTexture(int slot) {
	std::string command = "res = GetActionTexture(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	std::string result = (char*)Functions::GetText("res");
	return result;
}

bool FunctionsLua::IsConsumableAction(int slot) {
	std::string command = "res = IsConsumableAction(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("res"));
	if (result == 1) return true;
	else return false;
}

bool FunctionsLua::IsActionInRange(int slot) {
	std::string command = "res = IsActionInRange(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("res"));
	if (result == 1) return true;
	else return false;
}

bool FunctionsLua::SpellIsTargeting() {
	std::string command = "res = SpellIsTargeting()";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("res"));
	if (result == 1) return true;
	else return false;
}

void FunctionsLua::SpellStopTargeting() {
	std::string command = "SpellStopTargeting()";
	Functions::LuaCall(command.c_str());
}

int FunctionsLua::GetSlot(std::string spell_name, std::string slot_type) {
	//Execution: 2ms
	int slot = 0; int spell_id;
	SpellSlotData spell = GetSpellData(spell_name);
	if (spell.id > 0) {
		for (int i = 1; i < 120; i++) {
			if (HasAction(i) && (GetSpellTexture(spell.slot) == GetActionTexture(i))
				&& ((slot_type == "SPELL" && !IsConsumableAction(i))
				|| (slot_type == "ITEM" && IsConsumableAction(i)))) {
				slot = i;
			}
		}
	}
	return slot;
}

//======================================================================//
//=============================   Units   ==============================//
//======================================================================//

int FunctionsLua::UnitStat(std::string target, int nbr) {
	Functions::LuaCall(("stat = UnitStat(\"" + target + "\", " + std::to_string(nbr) + ")").c_str());
	int stat = GetIntFromChar((char*)Functions::GetText("stat"));
	return stat;
}

void FunctionsLua::TargetUnit(std::string target) {
	Functions::LuaCall(("TargetUnit(\"" + target + "\")").c_str());
}

std::string FunctionsLua::UnitName(std::string target) {
	Functions::LuaCall(("name = UnitName(\"" + target + "\")").c_str());
	std::string name = (char*)Functions::GetText("name");
	return name;
}

bool FunctionsLua::UnitCanAttack(std::string char1, std::string char2) {
	std::string command = "canAttack = UnitCanAttack(\"" + char1 + "\", \"" + char2 + "\")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("canAttack"));
	if (result == 1) return true;
	else return false;
}

bool FunctionsLua::UnitIsDeadOrGhost(std::string char1) {
	std::string command = "dead = UnitIsDeadOrGhost(\"" + char1 + "\")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("dead"));
	if (result == 1) return true;
	else return false;
}

bool FunctionsLua::CheckInteractDistance(std::string char1, int dist) {
	std::string command = "checkInteract = CheckInteractDistance(\"" + char1 + "\", " + std::to_string(dist) + ")";
	Functions::LuaCall(command.c_str());
	int result = GetIntFromChar((char*)Functions::GetText("checkInteract"));
	if (result == 1) return true;
	else return false;
}

bool FunctionsLua::UnitAffectingCombat(std::string target) {
	std::string command = "cb = UnitAffectingCombat(\"" + target + "\")";
	Functions::LuaCall(command.c_str());
	int cb = GetIntFromChar((char*)Functions::GetText("cb"));
	if (cb == 1) return true;
	else return false;
}

std::string FunctionsLua::UnitClass(std::string target) {
	Functions::LuaCall(("class = UnitClass(\"" + target + "\")").c_str());
	std::string tarClass = (char*)Functions::GetText("class");
	return tarClass;
}

bool FunctionsLua::UnitIsCaster(std::string target) {
	std::string tarClass = UnitClass(target);
	if (tarClass == "Priest" || tarClass == "Warlock" || tarClass == "Mage") return true;
	else if (target == "player" && ((tarClass == "Druid" && (playerSpec == 0 || playerSpec == 2)) || (tarClass == "Shaman" && (playerSpec == 0 || playerSpec == 2)))) return true;
	else return false;
}

bool FunctionsLua::UnitIsElite(std::string target) {
	Functions::LuaCall(("classification = UnitClassification(\"" + target + "\")").c_str());
	std::string classification = (char*)Functions::GetText("classification");
	Functions::LuaCall(("level = UnitLevel(\"" + target + "\")").c_str());
	int level = GetIntFromChar((char*)Functions::GetText("level"));
	Functions::LuaCall(("playerCtrl = UnitPlayerControlled(\"" + target + "\")").c_str());
	int playerCtrl = GetIntFromChar((char*)Functions::GetText("playerCtrl"));
	if (classification == "elite" || classification == "rareelite" || level == -1 || playerCtrl == 1) return true;
	else return false;
}

std::string FunctionsLua::UnitCreatureType(std::string target) {
	Functions::LuaCall(("creatureType = UnitCreatureType(\"" + target + "\")").c_str());
	std::string type = (char*)Functions::GetText("creatureType");
	return type;
}

bool FunctionsLua::IsGroupInCombat() {
	for (int i = 1; i <= NumGroupMembers; i++) {
		if(UnitAffectingCombat(tarType+std::to_string(i))) return true;
	}
	return false;
}

bool FunctionsLua::IsShieldEquipped() {
	Functions::LuaCall("_, _, id = string.find(GetInventoryItemLink(\"player\", GetInventorySlotInfo(\"SecondaryHandSlot\")) or \"\", \"(item:%d+:%d+:%d+:%d+)\")");
	int id = GetIntFromChar((char*)Functions::GetText("id"));
	if (id > 0) {
		std::string command = "_, _, _, _, itemType = GetItemInfo(" + std::to_string(id) + ")";
		Functions::LuaCall(command.c_str());
		std::string itemType = (char*)Functions::GetText("itemType");
		if (itemType == "Armor") return true;
	}
	return false;
}

bool FunctionsLua::HasWandEquipped() {
	Functions::LuaCall("wand = HasWandEquipped()");
	int wand = GetIntFromChar((char*)Functions::GetText("wand"));
	if (wand == 1) return true;
	else return false;
}

int FunctionsLua::GetComboPoints() {
	Functions::LuaCall("pts = GetComboPoints()");
	int pts = GetIntFromChar((char*)Functions::GetText("pts"));
	return pts;
}

int FunctionsLua::GetTalentInfo(int page, int index) {
	std::string command = "_,_,_,_,pts = GetTalentInfo(" + std::to_string(page) + ", " + std::to_string(index) + ")";
	Functions::LuaCall(command.c_str());
	int pts = GetIntFromChar((char*)Functions::GetText("pts"));
	return pts;
}

bool FunctionsLua::IsCurrentAction(int slot) {
	std::string command = "current = IsCurrentAction(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int current = GetIntFromChar((char*)Functions::GetText("current"));
	if (current == 1) return true;
	else return false;
}

float FunctionsLua::UnitAttackSpeed(std::string target) {
	std::string command = "aaspeed = UnitAttackSpeed(\"" + target + "\")";
	Functions::LuaCall(command.c_str());
	float aaspeed = GetFloatFromChar((char*)Functions::GetText("aaspeed"));
	return aaspeed;
}

void FunctionsLua::FollowUnit(std::string target) {
	std::string command = "FollowUnit(\"" + target + "\")";
	Functions::LuaCall(command.c_str());
}

//======================================================================//
//=============================   Gossip   =============================//
//======================================================================//

void FunctionsLua::SelectGossipOption(int index) {
	std::string command = "SelectGossipOption(\"" + std::to_string(index) + "\")";
	Functions::LuaCall(command.c_str());
}

void FunctionsLua::BuyTrainerService(int index) {
	std::string command = "BuyTrainerService(\"" + std::to_string(index) + "\")";
	Functions::LuaCall(command.c_str());
}