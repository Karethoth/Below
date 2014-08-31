#pragma once

#include "../events/event.hh"

#include <string>


struct JoinEvent : public Event
{
	unsigned int clientId;
};


struct PartEvent : public Event
{
	unsigned int clientId;
};


struct DataInEvent : public Event
{
	unsigned int clientId;
	std::string  data;
};


struct PingEvent : public Event
{
	unsigned int clientId;
	std::string  msg;
};


struct PongEvent : public Event
{
	unsigned int clientId;
	std::string  msg;
};

