#pragma once

#include "../events/event.hh"

#include <string>


struct ObjectCreateEvent : public Event
{
	unsigned char objectType;
	std::string data;
};


struct ObjectDestroyEvent : public Event
{
	unsigned int objectId;
};


struct ObjectUpdateEvent : public Event
{
	unsigned int objectId;
	std::string  data;
};

