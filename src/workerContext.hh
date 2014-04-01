#pragma once

#include <memory>
#include <thread>

#include "taskManager.hh"

struct WorkerContext
{
	std::thread::id threadId;
	TaskManager *taskManager;
	bool shouldStop;
};

