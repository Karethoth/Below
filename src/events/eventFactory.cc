#include "eventFactory.hh"


EventFactory::EventFactory()
{
	eventQueue = nullptr;
}


void EventFactory::SetEventQueue( EventQueue *newQueue )
{
	eventQueue = newQueue;
}

