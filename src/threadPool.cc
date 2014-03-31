#include "threadPool.hh"


void ThreadPool::CleanThreads()
{
	std::lock_guard<std::mutex> threadLock( threadListMutex );
	std::lock_guard<std::mutex> contextLock( contextListMutex );

	for( auto currThread  = threads.begin();
	          currThread != threads.end(); )
	{
		// If we find the context, we won't clean the thread.
		bool contextFound = false;
		for( auto currContext  = contexts.begin();
		          currContext != contexts.end();
		          currContext++ )
		{
			if( (*currContext)->threadId == (*currThread)->get_id() )
			{
				contextFound = true;
				break;
			}
		}

		if( contextFound )
		{
			currThread++;
			continue;
		}

		// If no context is found, we erase the thread
		currThread = threads.erase( currThread );
	}
}

