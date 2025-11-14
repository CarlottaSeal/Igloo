#pragma once
#include "ErrorWarningAssert.hpp"
#include "Engine/Core/NetworkSystem.h"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Job/JobSystem.h"
#include "Engine/Save/SaveSystem.h"
#include "Engine/UI/UISystem.h"

#define UNUSED(x) (void)(x);

extern NamedStrings g_gameConfigBlackboard;
extern EventSystem* g_theEventSystem;
extern DevConsole* g_theDevConsole;
extern UISystem* g_theUISystem;
extern NetworkSystem* g_theNetworkSystem;
extern SaveSystem* g_theSaveSystem;
extern JobSystem* g_theJobSystem;
