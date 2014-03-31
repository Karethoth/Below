#pragma once
#ifndef WORKER_CONTEXT_HH
#define WORKER_CONTEXT_HH

#include <memory>
#include <thread>

#include "taskManager.hh"

struct WorkerContext
{
	std::thread::id threadId;
	TaskManager *taskManager;
	bool shouldStop;
};


#endif

