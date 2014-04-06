#pragma once

#include <memory>
#include <thread>

#include "taskQueue.hh"


struct WorkerContext
{
	std::thread::id threadId;
	TaskQueue      *taskQueue;
	bool            shouldStop;
};

