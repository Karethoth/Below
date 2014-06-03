#pragma once

#include "../events/event.hh"

#include <string>


struct CreateEvent : public Event
{
	std::string data;
};


struct DestroyEvent : public Event
{
	unsigned int objectId;
};


struct UpdateEvent : public Event
{
	unsigned int objectId;
	std::string  data;
};

