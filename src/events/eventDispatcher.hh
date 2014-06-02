#pragma once

#include "event.hh"
#include "eventListener.hh"
#include "eventListenerCollection.hh"

#include <mutex>
#include <memory>

#include <vector>
#include <map>


class EventDispatcher
{
 public:
	EventDispatcher();
	~EventDispatcher();

	void HandleEvent( Event* );

	void AddEventListener( EventType, EventListenerPtr );

	std::mutex eventListenersMutex;
	std::map<EventType, EventListenerCollection*> eventListeners;
};

