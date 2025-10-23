#include "FunctionsLua.h"

#include "Game.h"
#include <iostream>

//======================================================================//
//=========================   Base Functions   =========================//
//======================================================================//

int GetIntFromChar(const char* txt) {
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

float GetFloatFromChar(const char* txt) {
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
		if (get<2>(item) == item_id) {
			std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(get<0>(item)) + +", " + std::to_string(get<1>(item)) + ")";
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
			if (get<2>(item) == items_id[i]) {
				std::string command = "start, duration = GetContainerItemCooldown(" + std::to_string(get<0>(item)) + +", " + std::to_string(get<1>(item)) + ")";
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
	int spell_id;
	std::tie(spell_id, std::ignore) = GetSpellID(spell_name);
	if (spell_id > 0) {
		std::string command = "start, duration = GetSpellCooldown(" + std::to_string(spell_id) + ", BOOKTYPE_SPELL)";
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
	for (const auto& item : virtualInventory) {
		if (GetItemQuality(get<3>(item)) == 0) {
			std::string command = "UseContainerItem(" + std::to_string(get<0>(item)) + ", " + std::to_string(get<1>(item)) + ")";
			Functions::LuaCall(command.c_str());
			return;
		}
	}
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

void FunctionsLua::MakeVirtualInventory(std::vector<std::tuple<int, int, int, std::string>>* listItems) {
	listItems->clear();
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string item_link = GetContainerItemLink(i, y);
			if (item_link.size() == 0) continue;
			int link_nbr = GetIntFromChar(item_link.c_str());
			listItems->push_back(std::make_tuple(i, y, link_nbr, item_link));
		}
	}
}

int FunctionsLua::GetContainerNumSlots(int slot) {
	std::string command = "slots = GetContainerNumSlots(" + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	int slots = GetIntFromChar((char*)Functions::GetText("slots"));
	return slots;
}

std::tuple<std::string, int> FunctionsLua::GetContainerItemInfo(int bag, int slot) {
	std::string command = "texture, itemCount = GetContainerItemInfo("+std::to_string(bag)+", "+std::to_string(slot)+")";
	Functions::LuaCall(command.c_str());
	std::string texture = (char*)Functions::GetText("texture");
	int itemCount = GetIntFromChar((char*)Functions::GetText("itemCount"));
	return std::make_tuple(texture, itemCount);
}

std::string FunctionsLua::GetContainerItemLink(int bag, int slot) {
	std::string command = "link = GetContainerItemLink(" + std::to_string(bag) + ", " + std::to_string(slot) + ")";
	Functions::LuaCall(command.c_str());
	std::string link = (char*)Functions::GetText("link");
	return link;
}

bool FunctionsLua::IsInventoryFull() {
	for (int i = 0; i <= 4; i++) {
		for (int y = 1; y <= GetContainerNumSlots(i); y++) {
			std::string texture;
			std::tie(texture, std::ignore) = GetContainerItemInfo(i, y);
			if (texture == "") return false;
		}
	}
	return true;
}

int FunctionsLua::GetItemCount(int item_id) {
	//Trouve par l'ID la quantit� d'item similaire dans l'inventaire
	int total = 0;
	for (const auto& item : virtualInventory) {
		if (get<2>(item) == item_id) {
			int itemCount;
			std::tie(std::ignore, itemCount) = GetContainerItemInfo(get<0>(item), get<1>(item));
			total = total + itemCount;
		}
	}
	return total;
}

bool FunctionsLua::HasItem(int* item_ids, int size) {
	//Trouve par l'ID si un des items de la liste est présent
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < size; i++) {
			if (get<2>(item) == item_ids[i]) return true;
		}
	}
	return false;
}

int FunctionsLua::GetItemQuality(std::string item_link) {
	if (item_link == "") return -1;
	else if (item_link.find("9d9d9d") != std::string::npos) return 0;	//Poor
	else if (item_link.find("ffffff") != std::string::npos) return 1;	//Common
	else if (item_link.find("1eff00") != std::string::npos) return 2;	//Uncommon
	else if (item_link.find("0070dd") != std::string::npos) return 3;	//Rare
	else if (item_link.find("a335ee") != std::string::npos) return 4;	//Epic
	else if (item_link.find("ff8000") != std::string::npos) return 5;	//Legendary
	else if (item_link.find("e6cc80") != std::string::npos) return 6;	//Artifact
	else return -1;
}

bool FunctionsLua::PickupItem(int item_id) {
	for (const auto& item : virtualInventory) {
		if (get<2>(item) == item_id) {
			std::string command = "PickupContainerItem(" + std::to_string(get<0>(item)) + ", " + std::to_string(get<1>(item)) + ")";
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

void FunctionsLua::PlaceItem(int slot, int item_id) {
	//Place l'objet dans le slot indiqu�
	PickupItem(item_id);
	std::string command = "PlaceAction(" + std::to_string(slot) + ")";
	Functions::LuaCall((command + " ClearCursor()").c_str());
}

void FunctionsLua::UseItem(int item_id) {
	//Use the indicated item
	for (const auto& item : virtualInventory) {
		if (get<2>(item) == item_id) {
			std::string command = "UseContainerItem(" + std::to_string(get<0>(item)) + ", " + std::to_string(get<1>(item)) + ")";
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
			if (get<2>(item) == listID[i]) return get<2>(item);
		}
	}
	return 0;
}

int FunctionsLua::HasMeat() {
	int CookedlistID[47] = { 117, 724, 1017, 2287, 2679, 2680, 2681, 2684, 2685, 2687, 2888, 3220, 3662, 3664, 3726, 3727, 3728, 3770, 3771, 4457, 4599, 5472, 5474, 5477, 5478, 5479, 7097, 8952, 11444, 12209, 12210, 12211, 12213, 12215, 12216, 12224, 13851, 17119, 17222, 17407, 17408, 18045, 19224, 19304, 19305, 20074, 21023 };
	for (const auto& item : virtualInventory) {
		for (int i = 0; i < 47; i++) {
			if (get<2>(item) == CookedlistID[i]) return CookedlistID[i];
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
			if (get<2>(item) == listID[i]) return true;
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
			if (get<2>(item) == listID[i]) return true;
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
			if (get<2>(item) == listID[i]) return true;
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
	std::string command = "texture,count,type = UnitDebuff(\"" + (std::string)target + "\", " + std::to_string(index) + ")";
	Functions::LuaCall(command.c_str());
	char* texture = (char*)Functions::GetText("texture");
	int count = GetIntFromChar((char*)Functions::GetText("count"));
	char* type = (char*)Functions::GetText("type");
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

std::tuple<int, int> FunctionsLua::GetSpellID(std::string spell_name, bool spell_exist) {
	//Execution = 1ms
	int id = 0; int rank = 0;
	for (int i = 1; i <= GetNumSpellTabs(); i++) {
		int numSpells;
		std::tie(std::ignore, std::ignore, std::ignore, numSpells) = GetSpellTabInfo(i);
		for (int y = 0; y < numSpells; y++) {
			id++;
			if (spell_name == GetSpellName(id)) {
				if (spell_exist) return std::make_tuple(id, 1);
				while (spell_name == GetSpellName(id + 1)) {
					id++; rank++;
				}
				return std::make_tuple(id, rank);
			}
		}
	}
	return std::make_tuple(0, 0);
}

bool FunctionsLua::IsPlayerSpell(std::string spell_name) {
	int spellID;
	std::tie(spellID, std::ignore) = GetSpellID(spell_name, true);
	if (spellID == 0) return false;
	else return true;
}

bool FunctionsLua::IsSpellReady(std::string spell_name) {
	//Execution: ~2.2ms
	if (((localPlayer->flags & UNIT_FLAG_SILENCED) != UNIT_FLAG_SILENCED)) {
		int slot = GetSlot(spell_name);
		if (slot > 0) {
			if (IsUsableAction(slot) && (GetActionCooldownDuration(slot) < 1.25f)) {
				return true;
			}
		}
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
	int slot = 0; int spellID;
	std::tie(spellID, std::ignore) = GetSpellID(spell_name);
	if (spellID > 0) {
		for (int i = 1; i < 120; i++) {
			if (HasAction(i) && (GetSpellTexture(spellID) == GetActionTexture(i))
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
	Functions::LuaCall(("type = UnitCreatureType(\"" + target + "\")").c_str());
	std::string type = (char*)Functions::GetText("type");
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
