#include <iostream>
#include <thread>
#include <vector>

#include "threadPool.hh"


void TestFunction()
{
	for( volatile unsigned int i=0; i < 200000; ++i )
		for( volatile unsigned int j=0; j < 10; ++j );
}



void WorkerLoop( WorkerContext *context, ThreadPool &pool )
{
	context->threadId = std::this_thread::get_id();

	// Run until thread should stop
	while( !context->shouldStop )
	{
		Task *task = context->taskManager->GetTask();
		if( !task )
		{
			std::this_thread::yield();
			std::this_thread::sleep_for( std::chrono::microseconds( 10 ) );
			continue;
		}

		// Execute the task
		task->f();

		// Delete the task when we're done with it
		delete task;
	}


	// Erase this context from the context list
	std::lock_guard<std::mutex> contextLock( pool.contextListMutex );
	for( auto currContext  = pool.contexts.begin();
	          currContext != pool.contexts.end();
	          currContext++ )
	{
		if( (*currContext) == context )
		{
			pool.contexts.erase( currContext );
			delete context;
			break;
		}
	}
}



int main()
{
	unsigned int hardwareThreads = std::thread::hardware_concurrency();

	ThreadPool  threadPool;
	TaskManager taskManager;


	// Create few test tasks
	for( int i = 0; i < 10000; ++i )
	{
		Task *t = new Task();
		t->f = TestFunction;
		t->dependencies = 0;
		taskManager.AddTask( t );
	}

	// Create a dependency critical task as a test and use anonymous functions.
	Task *dependent = new Task();
	dependent->dependencies = 2;
	dependent->f = []()
	{
		std::cout << "Executing the dependency critical task!" << std::endl;
	};

	Task *dep1 = new Task();
	dep1->dependencies = 0;
	dep1->f = []()
	{
		std::cout << "Dependency #1 satisfied\n";
	};
	dep1->dependents.push_back( dependent );

	Task *dep2 = new Task();
	dep2->dependencies = 0;
	dep2->f = []()
	{
		std::cout << "Dependency #2 satisfied\n";
	};
	dep2->dependents.push_back( dependent );


	taskManager.AddTask( dependent );
	taskManager.AddTask( dep2 );
	taskManager.AddTask( dep1 );


	// Create worker threads
	for( unsigned int i = 0; i < hardwareThreads; ++i )
	{
		// Create the context
		threadPool.contextListMutex.lock();
		WorkerContext *context = new WorkerContext();
		context->taskManager   = &taskManager;
		context->shouldStop    = false;
		threadPool.contexts.push_back( context );
		threadPool.contextListMutex.unlock();

		// Create the thread
		threadPool.threadListMutex.lock();
		std::thread *newThread = new std::thread( WorkerLoop, context, std::ref( threadPool ) );
		std::cout << "Created thread: " << newThread->get_id() << std::endl;
		threadPool.threads.push_back( newThread );
		threadPool.threadListMutex.unlock();
	}


	// Wait for tasks to end
	size_t taskCount;
	do
	{
		taskCount = taskManager.GetTaskCount();
		std::cout << "Task queue size: " << taskCount << std::endl;
		if( taskCount )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
		}
	}
	while( taskCount );


	// Command worker threads to stop
	threadPool.contextListMutex.lock();
	for( auto context  = threadPool.contexts.begin();
	          context != threadPool.contexts.end();
	          context++ )
	{
		(*context)->shouldStop = true;
	}
	threadPool.contextListMutex.unlock();


	// Wait for the thread pool to empty
	while( threadPool.threads.size() > 0 )
	{

		// Remove unjoinable threads
		threadPool.CleanThreads();

		std::this_thread::yield();
	}

	// Finish
	std::cout << "Finished." << std::endl;
	getc( stdin );

    return 0;
}

