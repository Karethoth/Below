#pragma once
#include "../statistics/executionTimer.hh"

#define EVENT_TYPE_LENGTH sizeof( unsigned char );
#define EVENT_SUB_TYPE_LENGTH sizeof( unsigned short );


enum EventType : unsigned char
{
	UNDEF_EVENT = 0,
	NETWORK_EVENT,
	STATE_EVENT,
	OBJECT_EVENT,
	SDL_INPUT_EVENT,
	SDL_WINDOW_EVENT,

	EVENT_TYPE_COUNT
};


enum EventSubType : unsigned short
{
	UNDEF_SUB_EVENT = 0,

	// Network events
	NETWORK_JOIN,
	NETWORK_PART,
	NETWORK_DATA_IN,

	// State events
	STATE_RUN_START,
	STATE_RUN_PAUSE,

	// Object events
	OBJECT_CREATE,
	OBJECT_DESTROY,
	OBJECT_UPDATE,
	OBJECT_PARENT_ADD,
	OBJECT_PARENT_REMOVE,
	OBJECT_CHILD_ADD,
	OBJECT_CHILD_REMOVE,

	// SDL input events
	SDL_TEXT_INPUT,
	SDL_TEXT_EDITING,
	SDL_JOYSTICK_INPUT,

	// SDL Window events
	SDL_WINDOW_RESIZE,
	SDL_WINDOW_FOCUS_CHANGE,

	EVENT_SUB_TYPE_COUNT
};



struct Event
{
	Event();

	EventType      type;
	EventSubType   subType;
	ExecutionTimer timer;
};

