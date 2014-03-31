#include "taskManager.hh"


Task* TaskManager::GetTask()
{
	std::lock_guard<std::mutex> taskLock( taskQueueMutex );

	Task *task = nullptr;

	if( taskQueue.size() <= 0 )
	{
		return task;
	}

	for( auto t = taskQueue.begin();
	          t != taskQueue.end();
	          t++ )
	{
		if( !(*t)->HasDependenciesLeft() )
		{
			task = (*t);
			taskQueue.erase( t );
			break;
		}
	}

	return task;
}



void TaskManager::AddTask( Task *newTask )
{
	std::lock_guard<std::mutex> taskLock( taskQueueMutex );
	taskQueue.push_back( newTask );
}



size_t TaskManager::GetTaskCount()
{
	return taskQueue.size();
}

