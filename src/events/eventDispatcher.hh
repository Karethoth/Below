#pragma once

#include "event.hh"

#include <mutex>
#include <memory>
#include <vector>
#include <map>


struct EventListener
{
	virtual void HandleEvent( Event* ) = 0;
};


typedef std::shared_ptr<EventListener> EventListenerPtr;


struct EventListenerCollection
{
	std::mutex collectionMutex;
	std::vector<EventListenerPtr> collection;
	void AddEventListener( EventListenerPtr );
};



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

