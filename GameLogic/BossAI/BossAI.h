#pragma once
#include "../Game.h"
#include "../Navigation.h"
#include "../MemoryManager.h"
#include "../Functions/FunctionsLua.h"

#include <time.h>

class BossAI {
public:
	static bool BossAIAction();
	static bool Gnomeregan();
	static bool Maraudon();
	static bool ScarletMonastery();
};