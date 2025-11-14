#include "EventSystem.hpp"
#include "NamedStrings.hpp"
#include "EngineCommon.hpp"

EventSystem* g_theEventSystem = nullptr;

EventSystem::EventSystem(EventSystemConfig const& config)
	: m_config(config)
{
}

EventSystem::~EventSystem()
{
}

void EventSystem::StartUp()
{
}

void EventSystem::Shutdown()
{
	m_subscriptionListByEventName.clear();
	m_registeredCommands.clear();
}

void EventSystem::BeginFrame()
{
}

void EventSystem::EndFrame()
{
}

void EventSystem::SubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
	m_subscriptionListByEventName[eventName].push_back({ functionPtr });
	if (std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName) == m_registeredCommands.end())
	{
		m_registeredCommands.push_back(eventName);
	}
}

void EventSystem::UnsubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	auto it = m_subscriptionListByEventName.find(eventName);
	if (it != m_subscriptionListByEventName.end()) 
	{
		SubscriptionList& subscriptions = it->second;
		subscriptions.erase(
			std::remove_if(subscriptions.begin(), subscriptions.end(),
				[&functionPtr](EventSubscription const& subscription)
				{
					return subscription.callback == functionPtr;
				}),
			subscriptions.end());

		if (subscriptions.empty()) //If this event's subscriptions are empty, remove the event name
		{
			m_subscriptionListByEventName.erase(it);

			m_registeredCommands.erase(
				std::remove(m_registeredCommands.begin(), m_registeredCommands.end(), eventName),
				m_registeredCommands.end());
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	// copy arrays to avoid call function when locked
	SubscriptionList subscriptionsCopy;
	bool commandExists = false;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
        
		auto commandIt = std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName);
		commandExists = (commandIt != m_registeredCommands.end());
        
		if (commandExists) {
			auto it = m_subscriptionListByEventName.find(eventName);
			if (it != m_subscriptionListByEventName.end())
			{
				subscriptionsCopy = it->second; 
			}
		}
	}
	if (!commandExists)
	{
		if (g_theDevConsole)
		{
			EventArgs eventArgs;
			eventArgs.SetValue("warningEvent", eventName);
			FireEvent("Warning", eventArgs); 
		}
		return;
	}
    
	for (const EventSubscription& subscription : subscriptionsCopy)
	{
		bool consumed = subscription.callback(args);
		if (consumed) {
			break;
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName)
{
	EventArgs emptyArgs;
	bool commandExists = false;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		auto it = std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName);
		commandExists = (it != m_registeredCommands.end());
	}
    
	if (commandExists)
	{
		FireEvent(eventName, emptyArgs); 
	}
	else
	{
		if (g_theDevConsole)
		{
			EventArgs eventArgs;
			eventArgs.SetValue("warningEvent", eventName);
			FireEvent("Warning", eventArgs);
		}
	}
}

Strings EventSystem::GetAllRegisteredCommands()
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	return m_registeredCommands;
}

void SubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr)
{
	if (g_theEventSystem != nullptr)
	{
		g_theEventSystem->SubscribeEventCallBackFunction(eventName, functionPtr);
	}
}

void UnsubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr)
{
	if (g_theEventSystem != nullptr)
	{
		g_theEventSystem->UnsubscribeEventCallBackFunction(eventName, functionPtr);
	}
}

void FireEvent(std::string const& eventName, EventArgs& args)
{
	if (g_theEventSystem != nullptr)
	{
		g_theEventSystem->FireEvent(eventName, args);
	}
}

void FireEvent(std::string const& eventName)
{
	if (g_theEventSystem != nullptr)
	{
		g_theEventSystem->FireEvent(eventName);
	}
}
