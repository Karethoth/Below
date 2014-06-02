#pragma once

#include "event.hh"

#include <memory>


struct EventListener
{
	virtual void HandleEvent( Event* ) = 0;
};


typedef std::shared_ptr<EventListener> EventListenerPtr;

