#include "taskQueue.hh"


Task* TaskQueue::GetTask()
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



void TaskQueue::AddTask( Task *newTask )
{
	std::lock_guard<std::mutex> taskLock( taskQueueMutex );
	taskQueue.push_back( newTask );
}



size_t TaskQueue::GetTaskCount()
{
	std::lock_guard<std::mutex> taskLock( taskQueueMutex );
	return taskQueue.size();
}

