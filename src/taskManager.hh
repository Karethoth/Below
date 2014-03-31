#pragma once
#ifndef TASK_MANAGER_HH
#define TASK_MANAGER_HH

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


#endif

