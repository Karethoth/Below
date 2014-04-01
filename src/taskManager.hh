#pragma once

#include <mutex>

#include "task.hh"


class TaskManager
{
 public:
	Task* GetTask();
	void  AddTask( Task* );

	size_t GetTaskCount();

 protected:
	std::mutex taskQueueMutex;
	std::vector<Task*> taskQueue;
};

