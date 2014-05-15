#include <iostream>
#include <thread>
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>

#include <exception>
#include <boost\asio.hpp>

#include "threadPool.hh"

#include "eventQueue.hh"
#include "eventDispatcher.hh"
#include "networkEvents.hh"


#ifdef __WIN32__
	#pragma comment( lib, "opengl32.lib" )

	#ifndef _DEBUG
		#pragma comment( lib, "glew32.lib" )
	#else
		#pragma comment( lib, "glew32d.lib" )
	#endif

	#undef main
#endif


using namespace std;
using boost::asio::ip::tcp;


// Some global queues, pools, etc.
ThreadPool      threadPool;
TaskQueue       taskQueue;
EventQueue      eventQueue;
EventDispatcher eventDispatcher;
boost::asio::io_service ioService;


bool stopClient = false;


void WorkerLoop( WorkerContext *context, ThreadPool &pool )
{
	context->threadId = std::this_thread::get_id();

	// Run until thread should stop
	while( !context->shouldStop )
	{
		Task *task = context->taskQueue->GetTask();
		if( !task )
		{
			std::this_thread::yield();
			continue;
		}


		// Execute the task
		if (task->f)
		{
			task->f();
		}

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


struct NetworkListener : public EventListener
{
	void HandleEvent( Event *e )
	{
		JoinEvent   *join;
		PartEvent   *part;
		DataInEvent *dataIn;

		switch( e->subType )
		{
			case NETWORK_JOIN:
				cout << "We joined!" << endl;
				break;

			case NETWORK_PART:
				cout << "We disconnected!" << endl;
				break;

			case NETWORK_DATA_IN:
				dataIn = static_cast<DataInEvent*>( e );
				cout << "Data in \"" << dataIn->data << "\"" << endl;
				break;

			default:
				cout << "undef(" << e->subType << "): " << endl;
		}
	}
};


void EventHandlerTask()
{
	// Get an event
	Event *e = eventQueue.GetEvent();
	if( !e )
	{
		return;
	}

	// Pass the event to the listeners
	eventDispatcher.HandleEvent( e );

	// Free the event
	delete e;
}



// Task to read from the server,
// generates itself again at the end
void ReaderTask()
{
	// TODO: add the actual reading part here

	// Generate the task for the next read:
	Task *readerTask = new Task();
	readerTask->dependencies = 0;
	readerTask->f = ReaderTask;
	taskQueue.AddTask( readerTask );
}



// Task to step time after time the I/O services,
// generates itself again at the end
void IoStepTask()
{
	// Run one step
	ioService.run_one();

	// Generate the next task
	Task *ioStepTask = new Task();
	ioStepTask->dependencies = 0;
	ioStepTask->f = IoStepTask;
	taskQueue.AddTask( ioStepTask );
}



int main( int argc, char **argv )
{
	// Get count of hardware threads
	unsigned int hardwareThreads = std::thread::hardware_concurrency();

	auto networkListener = make_shared<NetworkListener>();
	eventDispatcher.AddEventListener( NETWORK_EVENT, networkListener );


	// Try to connect to the server
	cout << "Connecting to the server..." << endl;
	try
	{
		// TODO: Connect
	}
	catch( std::exception &e )
	{
		cerr << "Failed to connect: " << e.what() << endl;
		cout << "Press enter to quit." << endl;
		getc( stdin );
		return 1;
	}
	cout << "Connected!" << endl;


	// Create the worker threads
	cout << "Creating worker threads" << endl;

	for( unsigned int i = 0; i < hardwareThreads; ++i )
	{
		// Create the context
		threadPool.contextListMutex.lock();
		WorkerContext *context = new WorkerContext();
		context->taskQueue     = &taskQueue;
		context->shouldStop    = false;
		threadPool.contexts.push_back( context );
		threadPool.contextListMutex.unlock();

		// Create the thread
		threadPool.threadListMutex.lock();
		std::thread *newThread = new std::thread( WorkerLoop, context, std::ref( threadPool ) );
		cout << "Created thread: " << newThread->get_id() << endl;
		threadPool.threads.push_back( newThread );
		threadPool.threadListMutex.unlock();
	}



	// Create a task to generate tasks to handle events
	cout << "Creating the event handler tasker." << endl;

	Task *eventTasker = new Task();
	eventTasker->dependencies = 0;
	eventTasker->f = []()
	{
		while( true )
		{
			if( eventQueue.GetEventCount() )
			{
				Task *eventTask = new Task();
				eventTask->dependencies = 0;
				eventTask->f = EventHandlerTask;
				taskQueue.AddTask( eventTask );
			}
			this_thread::yield();
		}
	};
	taskQueue.AddTask( eventTasker );


	// Create a task to handle reading from the server
	cout << "Creating the reader tasker." << endl;

	Task *readerTasker = new Task();
	readerTasker->dependencies = 0;
	readerTasker->f = ReaderTask;
	taskQueue.AddTask( readerTasker );


	// Create a task to run the network io services
	cout << "Creating the network I/O tasker." << endl;

	Task *ioTasker = new Task();
	ioTasker->dependencies = 0;
	ioTasker->f = IoStepTask;
	taskQueue.AddTask( ioTasker );



	// Main loop
	cout << "Starting the main loop" << endl;
	do
	{
		size_t taskCount  = taskQueue.GetTaskCount();
		size_t eventCount = eventQueue.GetEventCount();

		// Print out at least some stats
		cout << "------------------------" <<  endl;
		cout << "Task  queue size: " << taskCount  << endl;
		cout << "Event queue size: " << eventCount << endl;

		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	}
	while( !stopClient );
	cout << "Main loop ended!" << endl;


	// Command worker threads to stop
	cout << "Stopping the worker threads!" << endl;

	threadPool.contextListMutex.lock();
	for( auto context  = threadPool.contexts.begin();
	          context != threadPool.contexts.end();
	          context++ )
	{
		(*context)->shouldStop = true;
	}
	threadPool.contextListMutex.unlock();


	// Wait for the thread pool to empty
	cout << "Worker threads have been commanded to stop." << endl;

	while( threadPool.threads.size() > 0 )
	{

		// Remove unjoinable threads
		threadPool.CleanThreads();

		std::this_thread::yield();
	}
	cout << "Worker threads stopped!" << endl;


	// Finish
	std::cout << "Finished, press enter to quit." << std::endl;
	getc( stdin );

	return 0;
}

