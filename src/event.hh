#pragma once


enum EventType
{
	UNDEF_EVENT = 0,
	NETWORK_EVENT,
	STATE_EVENT,
	EVENT_TYPE_COUNT
};


enum EventSubType
{
	UNDEF_SUB_EVENT = 0,

	// Network events
	CLIENT_JOIN,
	CLIENT_PART,

	// State events
	STATE_PAUSE_START,
	STATE_PAUSE_END
};



struct Event
{
	EventType    type;
	EventSubType subType;
};

