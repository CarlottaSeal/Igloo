#pragma once
#include "ErrorWarningAssert.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/UI/UISystem.h"

#define UNUSED(x) (void)(x);

extern NamedStrings g_gameConfigBlackboard;
extern EventSystem* g_theEventSystem;
extern DevConsole* g_theDevConsole;
extern UISystem* g_theUISystem;
