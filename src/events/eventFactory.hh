#pragma once

#include "eventDispatcher.hh"
#include "eventQueue.hh"


class EventFactory
{
 public:
	EventFactory();
	void SetEventQueue( EventQueue *eventQueue );

 protected:
	EventQueue *eventQueue;
};

