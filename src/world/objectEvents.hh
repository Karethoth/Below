#pragma once

#include "../events/event.hh"
#include "../world/worldObjectTypes.hh"
#include <string>


struct ObjectCreateEvent : public Event
{
	WorldObjectType objectType;
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


struct ObjectParentAddEvent : public Event
{
	unsigned int objectId;
	unsigned int parentId;
};


struct ObjectParentRemoveEvent : public Event
{
	unsigned int objectId;
	unsigned int parentId;
};


struct ObjectChildAddEvent : public Event
{
	unsigned int objectId;
	unsigned int childId;
};


struct ObjectChildRemoveEvent : public Event
{
	unsigned int objectId;
	unsigned int childId;
};

