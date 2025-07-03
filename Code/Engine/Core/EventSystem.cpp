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
	m_subscriptionListByEventName[eventName].push_back({ functionPtr });
	if (std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName) == m_registeredCommands.end())
	{
		m_registeredCommands.push_back(eventName);
	}
}

void EventSystem::UnsubscribeEventCallBackFunction(std::string const& eventName, EventCallBackFunction functionPtr)
{
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
	auto commandIt = std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName);
	if (commandIt == m_registeredCommands.end())
	{
		if (g_theDevConsole)
		{
			EventArgs eventArgs;
			eventArgs.SetValue("warningEvent", eventName);
			FireEvent("Warning",eventArgs);
			/*g_theDevConsole->AddLine(g_theDevConsole->ERRORLINE, "Unknown Command: " + eventName);
			g_theDevConsole->m_inputText.clear();
			g_theDevConsole->m_insertionPointPos = 0;*/
		}
		return;
	}

	auto it = m_subscriptionListByEventName.find(eventName);
	if (it != m_subscriptionListByEventName.end())
	{
		SubscriptionList& subscriptions = it->second;
		for (const EventSubscription& subscription : subscriptions)
		{
			bool consumed = subscription.callback(args);
			if (consumed)
			{
				break;
			}
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName)
{
	EventArgs emptyArgs;
	auto it = std::find(m_registeredCommands.begin(), m_registeredCommands.end(), eventName);
	if (it != m_registeredCommands.end())
	{
		FireEvent(eventName, emptyArgs);
	}
	else
	{
		if (g_theDevConsole)
		{
			//g_theDevConsole->AddLine(g_theDevConsole->ERRORLINE, "Unknown Command: " + eventName);
			EventArgs eventArgs;
			eventArgs.SetValue("warningEvent", eventName);
			FireEvent("Warning", eventArgs);
		}
	}
}

Strings EventSystem::GetAllRegisteredCommands()
{
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
