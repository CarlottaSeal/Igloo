#pragma once
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "NamedStrings.hpp"

class NamedStrings;



typedef NamedStrings EventArgs;
//typedef std::function<bool(EventArgs&)> EventCallBackFunction;
typedef bool (*EventCallBackFunction)(EventArgs& args); // or you may alternatively use the new C++ “using” syntax for type aliasing

struct EventSubscription
{
	EventCallBackFunction callback;
};

typedef std::vector<EventSubscription> SubscriptionList;


struct EventSystemConfig
{
	//bool enableDebugLogging = false;
};

class EventSystem
{
public:
	EventSystem(EventSystemConfig const& config);
	~EventSystem();

	void StartUp();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void SubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr);
	void UnsubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr);
	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName);

	Strings GetAllRegisteredCommands();

protected:
	EventSystemConfig m_config;
	std::map<std::string, SubscriptionList> m_subscriptionListByEventName;
	Strings m_registeredCommands;
};

//standalone global-namespace helper functions, these forward to "the" event system, if it exists
void SubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr);
void UnsubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);