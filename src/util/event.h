#include "singleton.h"
#include <vector>
#include <map>
#include <functional>
using EventDescriptor = const char*;

class Event
{
public:
	static EventDescriptor __get_event_descriptor()
	{
		return "unknown";
	}
};

#define EVENT_CLASS( Eve ) class Eve : public Event {\
public: static EventDescriptor __get_event_descriptor() {return ""#Eve; }

#define EVENT_CLASS_END( Eve ) };

template<typename T>
struct EventDescriptorGetter
{
	static EventDescriptor Get()
	{
		static_assert(std::is_base_of_v<Event, T>, "class should be derived from base class event");
		return T::__get_event_descriptor();
	}
};

using EventObserver = std::function<void(const Event&)>;

class EventSystem : public Is_Signleton
{
public:
	EventSystem();

	template<typename EventT>
	void Register(EventObserver observer)
	{
		static_assert(std::is_base_of_v<Event, EventT>, "class should be derived from base class event");
		RegisterByDescriptor(EventDescriptorGetter<EventT>::Get(), observer);
	}

	template<typename EventT>
	void Dispatch(const EventT& eve)
	{
		static_assert(std::is_base_of_v<Event, EventT>, "class should be derived from base class event");
		DispatchByDescriptor(EventDescriptorGetter<EventT>::Get(), eve);
	}
	
private:
	void RegisterByDescriptor(EventDescriptor desc, EventObserver observer);

	void DispatchByDescriptor(EventDescriptor desc, const Event& eve);

	std::map<EventDescriptor, std::vector<EventObserver>> m_RegisteredObservers;
};

