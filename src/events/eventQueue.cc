#include "eventQueue.hh"

EventQueue::~EventQueue()
{
	/*
	for( auto it  = eventQueue.begin();
	          it != eventQueue.end();
	          it++ )
	{
		delete *it;
	}
	*/
}


Event* EventQueue::GetEvent()
{
	std::lock_guard<std::mutex> eventLock( eventQueueMutex );

	Event *event = nullptr;

	if( eventQueue.size() <= 0 )
	{
		return event;
	}

	event = *(eventQueue.begin());
	eventQueue.erase( eventQueue.begin() );

	return event;
}



void EventQueue::AddEvent( Event *newEvent )
{
	std::lock_guard<std::mutex> eventLock( eventQueueMutex );
	eventQueue.push_back( newEvent );
}



size_t EventQueue::GetEventCount()
{
	std::lock_guard<std::mutex> eventLock( eventQueueMutex );
	return eventQueue.size();
}

