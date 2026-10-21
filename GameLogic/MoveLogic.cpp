#include "Game.h"
#include "./Functions/FunctionsLua.h"
#include "Client.h"
#include "MemoryManager.h"
#include "ListAI.h"
#include "Navigation.h"
#include "FactionTemplate.h"

#include "./Data/SpellTrain.h"
#include "./Data/CraftTrain.h"

#include <iostream>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

void MoveToMap(Position targetpos) {
	Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, targetpos);
	float dist = nextpos.DistanceTo(localPlayer->position);
	if (dist > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
		ThreadSynchronizer::RunOnMainThread([nextpos]() {
			localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
		});
		Moving = MoveTypes::Journey;
	}
	else if (dist < 2.0f) { Moving = MoveTypes::NotMoving; }
}

void Game::CorpseRun() {
	if (localPlayer->isdead) {
		if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-11207.799805f, 1681.150024f, 60.143227f)) < 5.0f) {
			// Deadmines
			if (localPlayer->zoneID == 1581) {
				// Inside Hideout
				if (localPlayer->position.Z > 37) { MoveToMap(Position(-11105.501953f, 1486.848267f, 32.607071f)); }
				else { MoveToMap(Position(-11207.504883f, 1680.481934f, 24.358500f)); }
			}
			else { MoveToMap(Position(-11082.001953f, 1527.153931f, 43.320770f)); }
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-230.988998f, 1571.569946f, 97.390625f)) < 5.0f) {
			// Shadowfang
			MoveToMap(Position(-231.766205f, 1570.990845f, 76.892220f));
		}
		else if (mapID == 1 && localPlayer->corpse_position.DistanceTo(Position(4249.12f, 748.387f, 147.263f)) < 5.0f) {
			// Blackfathom Deeps
			if (localPlayer->zoneID == 719) { MoveToMap(Position(4247.740234f, 745.879028f, -24.296673f)); }
			else {
				if (localPlayer->position.DistanceTo(Position(4247.740234f, 745.879028f, -24.296673f)) < 5.0f) { ThreadSynchronizer::pressKey(0x26); }
				MoveToMap(Position(4119.196289f, 888.643982f, 9.724840f));
			}
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-5162.66f, 931.599f, 408.975f)) < 5.0f) {
			// Gnomeregan
			if (localPlayer->position.Z < 280.0f) MoveToMap(Position(-5162.885254f, 928.664246f, 257.180511f));
			else MoveToMap(Position(-5163.476563f, 659.317871f, 348.278412f));
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(2892.239990f, -811.263977f, 199.328125f)) < 5.0f) {
			// Scarlet Monastery
			MoveToMap(Position(2903.302002f, -805.908508f, 160.331848f));
		}
		else if (mapID == 0 && localPlayer->corpse_position.DistanceTo(Position(-6060.180176f, -2955.0f, 434.562317f)) < 5.0f) {
			// Uldaman
			MoveToMap(Position(-6063.834961f, -2954.836035f, 209.769699f));
		}
		else if (mapID == 1 && localPlayer->corpse_position.DistanceTo(Position(-6790.580078f, -2891.280029, 29.648438f)) < 5.0f) {
			// Zul'Farrak
			MoveToMap(Position(-6793.311035f, -2890.778076f, 8.887129f));
		}
		else if (mapID == 1 && localPlayer->corpse_position.DistanceTo(Position(-1432.699951f, 2924.979980f, 178.539062f)) < 5.0f) {
			// Maraudon
			MoveToMap(Position(-1475.534912f, 2617.167969f, 75.827087f));
		}
		else {
			std::cout << localPlayer->corpse_position.ToString() << "\n";
			Position nextpos = Navigation::CalculatePath(mapID, localPlayer->position, localPlayer->corpse_position);
			if (nextpos.DistanceTo(localPlayer->position) > 2.0f && !(localPlayer->movement_flags & MOVEFLAG_FORWARD)) {
				ThreadSynchronizer::RunOnMainThread([nextpos]() {
					localPlayer->ClickToMove(Move, localPlayer->Guid, nextpos);
				});
				Moving = MoveTypes::Journey;
			}
		}
	}
}

void SellItems() {
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Stormwind
		Position dest = Position(-8793.933594f, 701.635376f, 102.282463f);
		if (localPlayer->position.DistanceTo(dest) < 2.0f) {
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (ListUnits[i].entryID == 1297) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						Functions::InteractUnit(ListUnits[i].Pointer, 0);
						FunctionsLua::SelectGossipOption(1);
					});
					break;
				}
			}
		}
		else MoveToMap(dest);
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Ironforge
		Position dest = Position(-4794.620117f, -1120.365356f, 498.807526f);
		if (localPlayer->position.DistanceTo(dest) < 2.0f) {
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (ListUnits[i].entryID == 4259) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						Functions::InteractUnit(ListUnits[i].Pointer, 0);
					});
					break;
				}
			}
		}
		else MoveToMap(dest);
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Darnassus
		Position dest = Position(9922.597656f, 2317.589111f, 1330.787964f);
		if (localPlayer->position.DistanceTo(dest) < 2.0f) {
			for (unsigned int i = 0; i < ListUnits.size(); i++) {
				if (ListUnits[i].entryID == 4203) {
					ThreadSynchronizer::RunOnMainThread([i]() {
						Functions::InteractUnit(ListUnits[i].Pointer, 0);
					});
					break;
				}
			}
		}
		else MoveToMap(dest);
	}
}

void GoToTrainer(Position trainerPos, int trainerID) {
	if (localPlayer->position.DistanceTo(trainerPos) < 2.0f) {
		for (unsigned int i = 0; i < ListUnits.size(); i++) {
			if (ListUnits[i].entryID == trainerID) {
				ThreadSynchronizer::RunOnMainThread([i]() {
					Functions::InteractUnit(ListUnits[i].Pointer, 0);
					FunctionsLua::SelectGossipOption(1);
					FunctionsLua::BuyTrainerService(0);
				});
				break;
			}
		}
	}
	else MoveToMap(trainerPos);
}

bool TrainSpell() {
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Stormwind
		if (localPlayer->className == "Druid") { GoToTrainer(Position(-8774.315430f, 1096.036255f, 92.540367f), 5504); return true; }
		else if (localPlayer->className == "Hunter") { GoToTrainer(Position(-8415.759766f, 552.697998f, 95.531700f), 5515); return true; }
		else if (localPlayer->className == "Mage") {
			Position trainerPos = Position(-9010.751953f, 868.493286f, 29.620701f);
			if (localPlayer->position.DistanceTo(trainerPos) < 2.0f) {
				for (unsigned int i = 0; i < ListUnits.size(); i++) {
					if (ListUnits[i].entryID == 331) {
						ThreadSynchronizer::RunOnMainThread([i]() {
							Functions::InteractUnit(ListUnits[i].Pointer, 0);
							FunctionsLua::SelectGossipOption(1);
							FunctionsLua::BuyTrainerService(0);
						});
						break;
					}
				}
			}
			else if (trainerPos.DistanceTo(localPlayer->position) < 20.0f) MoveToMap(trainerPos);
			else MoveToMap(Position(-9015.769531f, 875.684570f, 148.616119f));
			return true;
		}
		else if (localPlayer->className == "Paladin") { GoToTrainer(Position(-8571.522461f, 863.083008f, 106.518623f), 5491); return true; }
		else if (localPlayer->className == "Priest") { GoToTrainer(Position(-8516.106445f, 859.622742f, 109.844681f), 376); return true; }
		else if (localPlayer->className == "Rogue") { GoToTrainer(Position(-8751.646484f, 380.214447f, 101.067060f), 918); return true; }
		else if (localPlayer->className == "Warlock") { GoToTrainer(Position(-8973.632813f, 1033.069702f, 101.404121f), 5495); return true; }
		else if (localPlayer->className == "Warrior") { GoToTrainer(Position(-8686.984375f, 323.889099f, 109.437485f), 5479); return true; }
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Ironforge
		if (localPlayer->className == "Hunter") { GoToTrainer(Position(-5012.019043f, -1273.012085f, 507.753845f), 5116); return true; }
		else if (localPlayer->className == "Mage") { GoToTrainer(Position(-4613.083008f, -927.388550f, 501.068207f), 5144); return true; }
		else if (localPlayer->className == "Paladin") { GoToTrainer(Position(-4601.649902f, -896.468018f, 502.850006f), 5149); return true; }
		else if (localPlayer->className == "Priest") { GoToTrainer(Position(-4617.910645f, -907.910706f, 501.070557f), 5142); return true; }
		else if (localPlayer->className == "Rogue") { GoToTrainer(Position(-4649.279785f, -1124.449707f, 508.549927f), 5167); return true; }
		else if (localPlayer->className == "Warlock") { GoToTrainer(Position(-4602.515137f, -1111.653076f, 504.939484f), 5172); return true; }
		else if (localPlayer->className == "Warrior") { GoToTrainer(Position(-5042.364258f, -1243.405273f, 507.754944f), 1901); return true; }
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Darnassus
		if (localPlayer->className == "Druid") { GoToTrainer(Position(10183.007812f, 2568.764648f, 1325.966064f), 4218); return true; }
		else if (localPlayer->className == "Hunter") { GoToTrainer(Position(10176.217773f, 2511.905518f, 1342.807129f), 4146); return true; }
		else if (localPlayer->className == "Priest") { GoToTrainer(Position(9655.435547f, 2535.975586f, 1331.519043f), 4091); return true; }
		else if (localPlayer->className == "Rogue") { GoToTrainer(Position(10084.106445f, 2548.979492f, 1294.893555f), 4163); return true; }
		else if (localPlayer->className == "Warrior") { GoToTrainer(Position(9940.208008f, 2282.287842f, 1341.394409f), 4089); return true; }
	}
	return false;
}

bool GoToVendor(Position vendorPos, int vendorID, int item_id, int item_quantity) {
	if (!vendorID) return false; // TODO: renseigner l'ID et la position du vendeur avant activation.
	if (localPlayer->position.DistanceTo(vendorPos) >= 2.0f) {
		MoveToMap(vendorPos);
		return true;
	}

	for (unsigned int i = 0; i < ListUnits.size(); i++) {
		if (ListUnits[i].entryID != vendorID) continue;

		ThreadSynchronizer::RunOnMainThread([i, item_id, item_quantity]() {
			Functions::InteractUnit(ListUnits[i].Pointer, 0);
			FunctionsLua::SelectGossipOption(1);
			int numItems = FunctionsLua::GetMerchantNumItems();
			for (int y = 1; y <= numItems; y++) {
				std::string item_link = FunctionsLua::GetMerchantItemLink(y);
				int item_sold_id = GetIntFromChar(item_link.c_str() + 11);
				if (item_id != item_sold_id) continue;
				FunctionsLua::BuyMerchantItem(y, item_quantity);
				return;
			}
			Functions::LuaCall("CloseMerchant()");
		});
		return true;
	}

	return true;
}

inline std::int64_t GetItemCount(int item_id) {
	std::int64_t count = 0;
	for (const auto& item : virtualInventory)
		if (item.id == item_id) count += item.count;
	return count;
}

inline const BuyableItem* FindBuyableItem(int item_id) {
	for (const auto& item : buyableItems)
		if (item.item_id == item_id) return &item;
	return nullptr;
}

inline std::optional<CraftPlan> TryPlanCraftBatch(ProfessionId profession_id, const Craft& craft, int max_crafts) {
	struct ResourceState {
		int item_id;
		int quantity;
		int unit_price;
		std::int64_t available;
	};

	std::vector<ResourceState> resources;
	resources.reserve(craft.resources.size());

	for (const auto& resource : craft.resources) {
		if (resource.item_quantity <= 0) return std::nullopt;
		const auto* buyable = FindBuyableItem(resource.item_id);
		resources.push_back({ resource.item_id, resource.item_quantity, buyable ? buyable->unit_price : -1, GetItemCount(resource.item_id) });
	}

	CraftPlan plan{ profession_id, craft.crafted_id, 0,{}, 0 };
	plan.to_buy.reserve(resources.size());

	for (int amount = max_crafts; amount >= 1; --amount) {
		plan.quantity = amount;
		plan.total_cost = 0;
		plan.to_buy.clear();
		bool possible = true;

		for (const auto& resource : resources) {
			std::int64_t required = static_cast<std::int64_t>(resource.quantity) * amount;
			std::int64_t missing = required - resource.available;
			if (missing <= 0) continue;

			if (resource.unit_price < 0 || missing >(std::numeric_limits<int>::max)()) { possible = false; break; }

			if (resource.unit_price > 0 && missing > (localPlayer->money - plan.total_cost) / resource.unit_price) { possible = false; break; }

			plan.to_buy.push_back({ resource.item_id, static_cast<int>(missing) });
			plan.total_cost += missing * resource.unit_price;
		}

		if (possible) return plan;
	}

	return std::nullopt;
}

inline bool IsEnchantRod(int item_id) {
	return item_id == 6218 || item_id == 6339 || item_id == 11130 || item_id == 11145 || item_id == 16207;
}

inline const Craft* FindEnchantRecipe(int crafted_id) {
	for (const auto& craft : enchantingCrafts)
		if (craft.crafted_id == crafted_id) return &craft;
	return nullptr;
}

inline int GetEnchantSpellId(int crafted_id) {
	const auto* recipe = FindEnchantRecipe(crafted_id);
	return recipe && recipe->spell_id ? recipe->spell_id : crafted_id;
}

inline bool MatchesEnchantCraftLink(const std::string& link, const Craft& recipe) {
	auto pos = link.find("enchant:");
	if (pos != std::string::npos) return std::atoi(link.c_str() + pos + 8) == (recipe.spell_id ? recipe.spell_id : recipe.crafted_id);
	pos = link.find("item:");
	return recipe.target_slot == 0 && pos != std::string::npos && std::atoi(link.c_str() + pos + 5) == recipe.crafted_id;
}

inline auto GetProfessionsToCheck() {
	return std::array{
		ProfessionToCheck{ ProfessionId::Leatherworking, leatherWorkingCrafts, localPlayer->leatherworkingLevel[0] },
		ProfessionToCheck{ ProfessionId::Mining, miningCrafts, localPlayer->miningLevel[0] },
		ProfessionToCheck{ ProfessionId::Blacksmithing, blacksmithCrafts, localPlayer->blacksmithingLevel[0] },
		ProfessionToCheck{ ProfessionId::Tailoring, tailoringCrafts, localPlayer->tailoringLevel[0] },
		ProfessionToCheck{ ProfessionId::Alchemy, alchemyCrafts, localPlayer->alchemyLevel[0] },
		ProfessionToCheck{ ProfessionId::Enchanting, enchantingCrafts, localPlayer->enchantingLevel[0] },
	};
}

inline std::optional<CraftPlan> GetCraftPlan(int max_crafts = 20) {
	if (!localPlayer || max_crafts <= 0) return std::nullopt;

	const auto professions = GetProfessionsToCheck();

	for (const auto& [id, crafts, level] : professions) {
		if (level <= 0) continue;
		for (const auto& craft : crafts) {
			if (level < craft.required_level || level >= craft.max_level) continue;
			if (id == ProfessionId::Enchanting && IsEnchantRod(craft.crafted_id) && GetItemCount(craft.crafted_id) > 0) continue;
			if (id == ProfessionId::Enchanting && craft.required_rod && GetItemCount(craft.required_rod) == 0) continue;
			if (id == ProfessionId::Enchanting && craft.spell_id == 0 && craft.target_slot == 0) continue;
			int batch_limit = (std::min)(max_crafts, craft.max_level - level);
			if (id == ProfessionId::Enchanting) batch_limit = 1;
			auto plan = TryPlanCraftBatch(id, craft, batch_limit);
			if (plan) return plan;
		}
	}
	return std::nullopt;
}

enum class CraftKnowledge { Checking, Learned, Missing };

struct CraftKnowledgeCache {
	std::mutex mutex;
	ProfessionId profession_id = ProfessionId::Leatherworking;
	int crafted_id = 0;
	CraftKnowledge status = CraftKnowledge::Checking;
	bool queued = false;
};

CraftKnowledgeCache craftKnowledgeCache;

inline const char* GetProfessionSpellName(ProfessionId profession_id) {
	switch (profession_id) {
		case ProfessionId::Leatherworking: return "Leatherworking";
		case ProfessionId::Blacksmithing: return "Blacksmithing";
		case ProfessionId::Mining: return "Smelting";
		case ProfessionId::Tailoring: return "Tailoring";
		case ProfessionId::Alchemy: return "Alchemy";
		case ProfessionId::Enchanting: return "Enchanting";
	}
	return nullptr;
}

inline const char* GetProfessionTradeSkillName(ProfessionId profession_id) {
	return profession_id == ProfessionId::Mining ? "Mining" : GetProfessionSpellName(profession_id);
}

inline void EnsureProfessionWindow(ProfessionId profession_id) {
	if (profession_id == ProfessionId::Enchanting) {
		Functions::LuaCall("if GetCraftName() ~= 'Enchanting' then CloseTradeSkill(); CloseCraft(); CastSpellByName('Enchanting') end");
		return;
	}
	const char* spell = GetProfessionSpellName(profession_id);
	const char* tradeSkill = GetProfessionTradeSkillName(profession_id);
	if (!spell || !tradeSkill) return;

	std::string lua = std::string("if GetTradeSkillLine() ~= '") + tradeSkill +
		"' then CloseCraft(); CloseTradeSkill(); CastSpellByName('" + spell + "') end";
	Functions::LuaCall(lua.c_str());
}

bool BuyItem(ProfessionId professionID, int item_id, int item_quantity) {
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Stormwind
		if (professionID == ProfessionId::Leatherworking || professionID == ProfessionId::Blacksmithing) { return GoToVendor(Position(-8722.855469f, 476.512756f, 98.613060f), 5565, item_id, item_quantity); }
		else if (professionID == ProfessionId::Mining) { return GoToVendor(Position(-8430.214844f, 622.596191f, 95.429405f), 5512, item_id, item_quantity); }
		else if (professionID == ProfessionId::Tailoring) { return GoToVendor(Position(-8940.175781f, 797.109863f, 90.941017f), 1347, item_id, item_quantity); }
		else if (professionID == ProfessionId::Alchemy) { return GoToVendor(Position(-8987.740234f, 751.758972f, 98.414497f), 1313, item_id, item_quantity); }
		else if (professionID == ProfessionId::Enchanting) { return GoToVendor(Position(-8854.016602f, 801.867798f, 96.334518f), 1318, item_id, item_quantity); }
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Darnassus
		if (professionID == ProfessionId::Leatherworking || professionID == ProfessionId::Blacksmithing) { return GoToVendor(Position(10082.842773f, 2262.257080f, 1343.310303f), 4225, item_id, item_quantity); }
		else if (professionID == ProfessionId::Tailoring) { return GoToVendor(Position(10086.066406f, 2258.160156f, 1333.000244f), 4168, item_id, item_quantity); }
		else if (professionID == ProfessionId::Alchemy) { return GoToVendor(Position(10064.889648f, 2343.624023f, 1321.567871f), 4226, item_id, item_quantity); }
		else if (professionID == ProfessionId::Enchanting) { return GoToVendor(Position(10134.400391f, 2318.459961f, 1333.079956f), 4228, item_id, item_quantity); }
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Ironforge
		if (professionID == ProfessionId::Leatherworking || professionID == ProfessionId::Blacksmithing) { return GoToVendor(Position(-4747.828125f, -1027.840332f, 504.429565f), 5128, item_id, item_quantity); }
		else if (professionID == ProfessionId::Mining) { return GoToVendor(Position(-4790.252441f, -1124.262573f, 498.806061f), 4259, item_id, item_quantity); }
		else if (professionID == ProfessionId::Tailoring) { return GoToVendor(Position(-4718.244629f, -1057.059814f, 504.196594f), 5154, item_id, item_quantity); }
		else if (professionID == ProfessionId::Alchemy) { return GoToVendor(Position(-4859.929688f, -1238.931030f, 501.722473f), 5178, item_id, item_quantity); }
		else if (professionID == ProfessionId::Enchanting) { return GoToVendor(Position(-4804.781250f, -1192.949463f, 505.814331f), 5158, item_id, item_quantity); }
	}
	return false;
}

bool TrainTradeSkill(ProfessionId professionID, bool higherRank = false) {
	Position trainerPos = localPlayer->position;
	int trainerID = 0;
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Stormwind
		if (professionID == ProfessionId::Leatherworking) {
			if (localPlayer->leatherworkingLevel[0] < 75 && !(localPlayer->leatherworkingLevel[0] >= 50 && localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(-8721.037109f, 469.952026f, 98.614304f); trainerID = 11096; }
			else if (localPlayer->leatherworkingLevel[0] < 150 && (!higherRank || localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(-8722.855469f, 476.512756f, 98.613060f); trainerID = 5564; }
		}
		else if (professionID == ProfessionId::Blacksmithing) {
			if (localPlayer->blacksmithingLevel[0] < 75 && !(localPlayer->blacksmithingLevel[0] >= 50 && localPlayer->blacksmithingLevel[1] <= 75)) { trainerPos = Position(-8427.233398f, 609.402161f, 95.222618f); trainerID = 957; }
			else if (localPlayer->blacksmithingLevel[0] < 150 && (!higherRank || localPlayer->blacksmithingLevel[1] <= 75)) { trainerPos = Position(-8427.312500f, 619.320312f, 95.453270f); trainerID = 5511; }
		}
		else if (professionID == ProfessionId::Mining) {
			trainerPos = Position(-8433.015625f, 696.366699f, 103.364143f); trainerID = 5513;
		}
		else if (professionID == ProfessionId::Tailoring) {
			if (localPlayer->tailoringLevel[0] < 75 && !(localPlayer->tailoringLevel[0] >= 50 && localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(-8940.175781f, 797.109863f, 90.941017f); trainerID = 1300; }
			else if (localPlayer->tailoringLevel[0] < 150 && (!higherRank || localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(-8940.175781f, 797.109863f, 90.941017f); trainerID = 1346; }
			else if (localPlayer->tailoringLevel[0] < 225 && (!higherRank || localPlayer->tailoringLevel[1] <= 150)) { trainerPos = Position(-8940.175781f, 797.109863f, 90.941017f); trainerID = 1346; }
		}
		else if (professionID == ProfessionId::Alchemy) {
			if (localPlayer->alchemyLevel[0] < 75 && !(localPlayer->alchemyLevel[0] >= 50 && localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(-8987.740234f, 751.758972f, 98.414497f); trainerID = 5500; }
			else if (localPlayer->alchemyLevel[0] < 150 && (!higherRank || localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(-8987.740234f, 751.758972f, 98.414497f); trainerID = 5499; }
		}
		else if (professionID == ProfessionId::Enchanting) {
			if (localPlayer->enchantingLevel[0] < 75 && !(localPlayer->enchantingLevel[0] >= 50 && localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(-8854.016602f, 801.867798f, 96.334518f); trainerID = 11068; }
			else if (localPlayer->enchantingLevel[0] < 150 && (!higherRank || localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(-8854.016602f, 801.867798f, 96.334518f); trainerID = 1317; }
			else if (localPlayer->enchantingLevel[0] < 225 && (!higherRank || localPlayer->enchantingLevel[1] <= 150)) { trainerPos = Position(-9573.968750f, -716.192200f, 99.125626f); trainerID = 11072; }
		}
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Darnassus
		if (professionID == ProfessionId::Leatherworking) {
			if (localPlayer->leatherworkingLevel[0] < 75 && !(localPlayer->leatherworkingLevel[0] >= 50 && localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(10087.839844f, 2254.337646f, 1343.310303f); trainerID = 11083; }
			else if (localPlayer->leatherworkingLevel[0] < 150 && (!higherRank || localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(10087.839844f, 2254.337646f, 1343.310303f); trainerID = 11081; }
			else if (localPlayer->leatherworkingLevel[0] < 225 && (!higherRank || localPlayer->leatherworkingLevel[1] <= 150)) { trainerPos = Position(10087.839844f, 2254.337646f, 1343.310303f); trainerID = 4212; }
		}
		else if (professionID == ProfessionId::Tailoring) {
			if (localPlayer->tailoringLevel[0] < 75 && !(localPlayer->tailoringLevel[0] >= 50 && localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(10084.529297f, 2266.178223f, 1333.000244f); trainerID = 11050; }
			else if (localPlayer->tailoringLevel[0] < 150 && (!higherRank || localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(10084.529297f, 2266.178223f, 1333.000244f); trainerID = 4159; }
		}
		else if (professionID == ProfessionId::Alchemy) {
			if (localPlayer->alchemyLevel[0] < 75 && !(localPlayer->alchemyLevel[0] >= 50 && localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(10073.293945f, 2355.213135f, 1321.582153f); trainerID = 11041; }
			else if (localPlayer->alchemyLevel[0] < 150 && (!higherRank || localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(10064.889648f, 2343.624023f, 1321.567871f); trainerID = 11042; }
			else if (localPlayer->alchemyLevel[0] < 225 && (!higherRank || localPlayer->alchemyLevel[1] <= 150)) { trainerPos = Position(10073.293945f, 2355.213135f, 1321.582153f); trainerID = 4160; }
		}
		else if (professionID == ProfessionId::Enchanting) {
			if (localPlayer->enchantingLevel[0] < 75 && !(localPlayer->enchantingLevel[0] >= 50 && localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(10144.823242f, 2317.142334f, 1333.000366f); trainerID = 11070; }
			else if (localPlayer->enchantingLevel[0] < 150 && (!higherRank || localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(10144.823242f, 2317.142334f, 1333.000366f); trainerID = 4213; }
		}
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
		// Ironforge
		if (professionID == ProfessionId::Leatherworking) {
			if (localPlayer->leatherworkingLevel[0] < 75 && !(localPlayer->leatherworkingLevel[0] >= 50 && localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(-4740.405762f, -1022.217102f, 504.428131f); trainerID = 1466; }
			else if (localPlayer->leatherworkingLevel[0] < 150 && (!higherRank || localPlayer->leatherworkingLevel[1] <= 75)) { trainerPos = Position(-4747.828125f, -1027.840332f, 504.429565f); trainerID = 5127; }
		}
		else if (professionID == ProfessionId::Blacksmithing) {
			if (localPlayer->blacksmithingLevel[0] < 75 && !(localPlayer->blacksmithingLevel[0] >= 50 && localPlayer->blacksmithingLevel[1] <= 75)) { trainerPos = Position(-4790.252441f, -1124.262573f, 498.806061f); trainerID = 10277; }
			else if (localPlayer->blacksmithingLevel[0] < 150 && (!higherRank || localPlayer->blacksmithingLevel[1] <= 75)) { trainerPos = Position(-4790.252441f, -1124.262573f, 498.806061f); trainerID = 10276; }
			else if (localPlayer->blacksmithingLevel[0] < 225 && (!higherRank || localPlayer->blacksmithingLevel[1] <= 150)) { trainerPos = Position(-4786.446289f, -1126.741211f, 498.805603f); trainerID = 4258; }
		}
		else if (professionID == ProfessionId::Mining) {
			trainerPos = Position(-4707.959961f, -1108.910034f, 504.528992f); trainerID = 4254;
		}
		else if (professionID == ProfessionId::Tailoring) {
			if (localPlayer->tailoringLevel[0] < 75 && !(localPlayer->tailoringLevel[0] >= 50 && localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(-4718.244629f, -1057.059814f, 504.196594f); trainerID = 1703; }
			else if (localPlayer->tailoringLevel[0] < 150 && (!higherRank || localPlayer->tailoringLevel[1] <= 75)) { trainerPos = Position(-4724.120117, -1054.709961f, 504.279999f); trainerID = 5153; }
		}
		else if (professionID == ProfessionId::Alchemy) {
			if (localPlayer->alchemyLevel[0] < 75 && !(localPlayer->alchemyLevel[0] >= 50 && localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(-4859.929688f, -1238.931030f, 501.722473f); trainerID = 1246; }
			else if (localPlayer->alchemyLevel[0] < 150 && (!higherRank || localPlayer->alchemyLevel[1] <= 75)) { trainerPos = Position(-4859.929688f, -1238.931030f, 501.722473f); trainerID = 5177; }
		}
		else if (professionID == ProfessionId::Enchanting) {
			if (localPlayer->enchantingLevel[0] < 75 && !(localPlayer->enchantingLevel[0] >= 50 && localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(-4804.781250f, -1192.949463f, 505.814331f); trainerID = 11065; }
			else if (localPlayer->enchantingLevel[0] < 150 && (!higherRank || localPlayer->enchantingLevel[1] <= 75)) { trainerPos = Position(-4809.029785f, -1186.130005f, 512.564026f); trainerID = 5157; }
		}
	}
	if (!trainerID) return false;
	GoToTrainer(trainerPos, trainerID);
	return true;
}

CraftKnowledge CheckCraftKnowledge(ProfessionId profession_id, int crafted_id) {
	int level = 0, maxLevel = 0;
	switch (profession_id) {
	case ProfessionId::Leatherworking: level = localPlayer->leatherworkingLevel[0]; maxLevel = localPlayer->leatherworkingLevel[1]; break;
	case ProfessionId::Blacksmithing: level = localPlayer->blacksmithingLevel[0]; maxLevel = localPlayer->blacksmithingLevel[1]; break;
	case ProfessionId::Mining: level = localPlayer->miningLevel[0]; maxLevel = localPlayer->miningLevel[1]; break;
	case ProfessionId::Tailoring: level = localPlayer->tailoringLevel[0]; maxLevel = localPlayer->tailoringLevel[1]; break;
	case ProfessionId::Alchemy: level = localPlayer->alchemyLevel[0]; maxLevel = localPlayer->alchemyLevel[1]; break;
	case ProfessionId::Enchanting: level = localPlayer->enchantingLevel[0]; maxLevel = localPlayer->enchantingLevel[1]; break;
	}
	if ((
		((level >= 50 && maxLevel <= 75) && (localPlayer->money >= 500) && (localPlayer->level >= 10)) ||
		((level >= 125 && maxLevel <= 150) && (localPlayer->money >= 5000) && (localPlayer->level >= 20)) ||
		((level >= 200 && maxLevel <= 225) && (localPlayer->money >= 50000) && (localPlayer->level >= 35))
		)) {
		if (TrainTradeSkill(profession_id, true)) return CraftKnowledge::Checking;
	}
	{
		std::lock_guard<std::mutex> lock(craftKnowledgeCache.mutex);
		if (craftKnowledgeCache.profession_id != profession_id || craftKnowledgeCache.crafted_id != crafted_id) {
			craftKnowledgeCache.profession_id = profession_id;
			craftKnowledgeCache.crafted_id = crafted_id;
			craftKnowledgeCache.status = CraftKnowledge::Checking;
			craftKnowledgeCache.queued = false;
		}
		if (craftKnowledgeCache.queued) return CraftKnowledge::Checking;
		if (craftKnowledgeCache.status != CraftKnowledge::Checking) {
			CraftKnowledge result = craftKnowledgeCache.status;
			craftKnowledgeCache.status = CraftKnowledge::Checking;
			return result;
		}
		craftKnowledgeCache.queued = true;
	}

	ThreadSynchronizer::RunOnMainThread([profession_id, crafted_id]() {
		CraftKnowledge status = CraftKnowledge::Checking;
		if (profession_id == ProfessionId::Enchanting) {
			if (FunctionsLua::GetCraftName() != "Enchanting") EnsureProfessionWindow(profession_id);
			else {
				Functions::LuaCall("for i=GetNumCrafts(),1,-1 do local _,_,typ,_,expanded=GetCraftInfo(i) if typ=='header' and not expanded then ExpandCraftSkillLine(i) end end");
				int count = FunctionsLua::GetNumCrafts();
				if (count > 0) {
					status = CraftKnowledge::Missing;
					const Craft* recipe = FindEnchantRecipe(crafted_id);
					for (int i = 1; recipe && i <= count; i++) {
						if (!MatchesEnchantCraftLink(FunctionsLua::GetCraftItemLink(i), *recipe)) continue;
						status = CraftKnowledge::Learned;
						break;
					}
				}
			}
		}
		else {
			EnsureProfessionWindow(profession_id);
			int count = FunctionsLua::GetNumTradeSkills();
			if (count > 0) {
				status = CraftKnowledge::Missing;
				for (int i = 1; i <= count; i++) {
					std::string link = FunctionsLua::GetTradeSkillItemLink(i);
					if (link.size() <= 11 || GetIntFromChar(link.c_str() + 11) != crafted_id) continue;
					status = CraftKnowledge::Learned;
					break;
				}
			}
		}

		std::lock_guard<std::mutex> lock(craftKnowledgeCache.mutex);
		if (craftKnowledgeCache.profession_id != profession_id || craftKnowledgeCache.crafted_id != crafted_id) return;
		craftKnowledgeCache.status = status;
		craftKnowledgeCache.queued = false;
	});
	return CraftKnowledge::Checking;
}

bool EnchantItem(int crafted_id) {
	const Craft* recipe = FindEnchantRecipe(crafted_id);
	if (!recipe || (recipe->spell_id == 0 && recipe->target_slot == 0)) return false;
	if (recipe->required_rod && GetItemCount(recipe->required_rod) == 0) return false;
	ThreadSynchronizer::RunOnMainThread([crafted_id]() {
		const Craft* recipe = FindEnchantRecipe(crafted_id);
		if (!recipe) return;
		FunctionsLua::EnchantCraft(recipe->spell_id ? recipe->spell_id : crafted_id, recipe->target_slot);
	});
	return true;
}

bool CraftItem(ProfessionId professionID, int item_id, int amount) {
	if (professionID == ProfessionId::Enchanting) return EnchantItem(item_id);
	ThreadSynchronizer::RunOnMainThread([professionID, item_id, amount]() {
		EnsureProfessionWindow(professionID);
		int numTradeSkills = FunctionsLua::GetNumTradeSkills();
		if (numTradeSkills == 0) return;

		for (int i = 1; i <= numTradeSkills; i++) {
			std::string item_link = FunctionsLua::GetTradeSkillItemLink(i);
			if (item_link.size() <= 11) continue;
			int item_skill_id = GetIntFromChar(item_link.c_str() + 11);
			if (item_id != item_skill_id) continue;
			FunctionsLua::DoTradeSkill(i, amount);
			return;
		}
	});
	return true;
}

inline bool MakeCraft() {
	if (inventoryFull) return false;
	auto plan = GetCraftPlan();
	if (!plan) return false;

	switch (CheckCraftKnowledge(plan->profession_id, plan->crafted_id)) {
	case CraftKnowledge::Checking: return true;
	case CraftKnowledge::Missing: return TrainTradeSkill(plan->profession_id);
	case CraftKnowledge::Learned: break;
	}

	if (plan->profession_id == ProfessionId::Blacksmithing && GetItemCount(5956) == 0) return false;

	if (!plan->to_buy.empty()) {
		for (const auto& item : plan->to_buy)
			if (!BuyItem(plan->profession_id, item.item_id, item.item_quantity)) return false;
		return true;
	}
	if (plan->profession_id == ProfessionId::Blacksmithing || plan->profession_id == ProfessionId::Mining) {
		Position anvilPos = Position(0, 0, 0);

		if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
			// Stormwind
			anvilPos = Position(-8427.233398f, 609.402161f, 95.222618f);
		}
		else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1) && (FactionTemplate.IsAlliance(localPlayer->factionTemplateID))) {
			// Ironforge
			anvilPos = Position(-4791.228516f, -1123.044312f, 498.806091f);
		}
		else return false;

		if (localPlayer->position.DistanceTo(anvilPos) >= 2.0f) { MoveToMap(anvilPos); return true; }
	}
	return CraftItem(plan->profession_id, plan->crafted_id, plan->quantity);
}

void AssembleBack() {
	if (mapID == 0 && (localPlayer->zoneID == 1519 || localPlayer->zoneID == 12)) {
		// Stormwind
		Position mageDimension = Position(-9019.012695f, 888.367188f, 29.620701f);
		if (localPlayer->position.DistanceTo(mageDimension) < 25.0f) MoveToMap(Position(-9019.012695f, 888.367188f, 29.620701f));
		else MoveToMap(Position(-8830.489258f, 625.553711f, 93.943695f));
	}
	else if (mapID == 0 && (localPlayer->zoneID == 1537 || localPlayer->zoneID == 1)) {
		// Ironforge
		MoveToMap(Position(-4756.274902f, -1154.143677f, 502.211273f));
	}
	else if (mapID == 1 && (localPlayer->zoneID == 1657 || localPlayer->zoneID == 141)) {
		// Darnassus
		MoveToMap(Position(9952.135742f, 2280.563965f, 1341.394409f));
	}
}

void Game::DoChores() {
	if (!localPlayer || localPlayer->channelInfo != 0 || localPlayer->castInfo != 0) return;
	bool hasItemToSell = false;
	int craftItems[] = { 2901 ,5956, 7005, };
	for (const auto& item : virtualInventory) {
		if (item.quality == 0 || ((item.type == "Weapon" || item.type == "Armor") && item.quality <= 3 && item.minLevel < localPlayer->level)) {
			bool craftItem = false;
			for (int i = 0; i < 3; i++) {
				if (item.id == craftItems[i]) {
					craftItem = true;
					break;
				}
			}
			if (!craftItem) hasItemToSell = true;
		}
	}
	if (!Combat && Disenchant()) {}
	else if (HasSpellToTrain() && TrainSpell()) {}
	else if (MakeCraft()) {}
	else if (hasItemToSell) SellItems();
	else AssembleBack();
}