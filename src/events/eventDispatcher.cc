#include "eventDispatcher.hh"


EventDispatcher::EventDispatcher()
{
	// Generate collections
	unsigned int type = UNDEF_EVENT;
	eventListenersMutex.lock();
	while( type < EVENT_TYPE_COUNT )
	{
		eventListeners[(EventType)type] = new EventListenerCollection();
		type++;
	}
	eventListenersMutex.unlock();
}


EventDispatcher::~EventDispatcher()
{
	eventListeners.clear();
}



void EventDispatcher::HandleEvent( Event *e )
{
	EventListenerCollection *listeners;
	eventListenersMutex.lock();
	listeners = eventListeners[e->type];
	eventListenersMutex.unlock();

	listeners->collectionMutex.lock();

	for( auto it  = listeners->collection.begin();
	          it != listeners->collection.end();
	          it++ )
	{
		(*it)->HandleEvent( e );
	}

	listeners->collectionMutex.unlock();
}


void EventDispatcher::AddEventListener( EventType type, EventListenerPtr listener )
{
	EventListenerCollection *listeners;

	eventListenersMutex.lock();
	listeners = eventListeners[type];
	eventListenersMutex.unlock();

	listeners->collectionMutex.lock();
	listeners->collection.push_back( listener );
	listeners->collectionMutex.unlock();
}

