#include "event.h"


EventSystem::EventSystem()
{

}

void EventSystem::RegisterByDescriptor(EventDescriptor desc, EventObserver observer)
{
	auto target = m_RegisteredObservers.find(desc);
	if (target != m_RegisteredObservers.end())
	{
		target->second.push_back(observer);
	}
	else
	{
		m_RegisteredObservers[desc] = std::vector<EventObserver>{ observer };
	}
}

void EventSystem::DispatchByDescriptor(EventDescriptor desc, const Event& eve)
{
	auto target = m_RegisteredObservers.find(desc);
	if (target != m_RegisteredObservers.end())
	{
		for (auto& obs : target->second) 
		{
			obs(eve);
		}
	}
}
