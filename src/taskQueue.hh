#pragma once

#include <mutex>

#include "task.hh"


class TaskQueue
{
 public:
	Task* GetTask();
	void  AddTask( Task* );

	size_t GetTaskCount();


 protected:
	std::mutex         taskQueueMutex;
	std::vector<Task*> taskQueue;
};

