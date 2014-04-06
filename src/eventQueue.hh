#pragma once

#include <vector>
#include <mutex>

#include "event.hh"


class EventQueue
{
  public:
	~EventQueue();

	Event* GetEvent();
	void   AddEvent( Event* );

	size_t GetEventCount();


  private:
	std::mutex          eventQueueMutex;
	std::vector<Event*> eventQueue;
};

