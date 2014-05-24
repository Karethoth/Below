#include <iostream>
#include <thread>
#include <vector>
#include <exception>
#include <csignal>

#include <boost/asio.hpp>


#include "../threadPool.hh"
#include "../network/server.hh"

#include "../events/eventQueue.hh"
#include "../events/eventDispatcher.hh"


using namespace std;
using boost::asio::ip::tcp;


// Some global queues, pools, etc.
ThreadPool      threadPool;
TaskQueue       taskQueue;
EventQueue      eventQueue;
EventDispatcher eventDispatcher;
Server          server;
boost::asio::io_service ioService;

std::atomic<unsigned int> eventHandlerTasks;

// Server stopper
bool stopServer = false;
bool ignoreLastThread = false;



void WorkerLoop( WorkerContext *context, ThreadPool &pool )
{
	context->threadId = std::this_thread::get_id();

	// Run until thread should stop
	while( !context->shouldStop )
	while( !context->shouldStop )
	{
		Task *task = context->taskQueue->GetTask();
		if( !task )
		{
			std::this_thread::yield();
			continue;
		}

		// Mark task as started
		task->timer.Start();

		// Execute the task
		if( task->f )
		{
			task->f();
		}

		// Mark task as ended
		task->timer.End();


		//ExecutionTimer timer = task->timer;
		//cout << task->name << ": Wait( " << timer.waitDuration.count()
		//     << " ) \tDuration( " << timer.executionDuration.count() << " )" << endl;

		// Delete the task when we're done with it
		delete task;

		std::this_thread::sleep_for( std::chrono::microseconds( 10 ) );
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
				cout << "Client joined!" << endl;
				break;

			case NETWORK_PART:
				cout << "Client parted!" << endl;
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

	eventHandlerTasks--;
}



// The generator of Event handler tasks
void EventHandlerGenerator()
{
	while( eventQueue.GetEventCount() > eventHandlerTasks )
	{
		Task *eventTask = new Task(
			std::string( "EventHandlerTask" ),
			EventHandlerTask
		);

		eventHandlerTasks++;
	}

	Task *eventTasker = new Task(
		std::string( "EventHandlerGenerator" ),
		EventHandlerGenerator
	);
	taskQueue.AddTask( eventTasker );
}



// Task to step time after time the I/O services,
// generates itself again at the end
void IoStepTask()
{
	// Run one step
	ioService.run_one();

	// Generate the next task
	Task *nextStep = new Task(
		std::string( "IoStepTask" ),
		IoStepTask
	);

	taskQueue.AddTask( nextStep );
}


// Signal handler for few possible events
void SignalHandler( int sig )
{
	stopServer       = true;
	ignoreLastThread = true;
}



bool GenerateWorkerThreads( unsigned int count )
{
	// Create the worker threads
	cout << "Creating worker threads" << endl;

	for( unsigned int i = 0; i < count; ++i )
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

	return true;
}



void GenerateVitalTasks()
{
	// Create a task to generate tasks to handle events
	cout << "Creating the event handler generator." << endl;

	Task *eventTasker = new Task(
		std::string( "EventHandlerGenerator" ),
		EventHandlerGenerator
	);
	taskQueue.AddTask( eventTasker );


	// Create a task to run the network io services
	cout << "Creating the network I/O tasker." << endl;

	Task *ioTasker = new Task(
		std::string( "IoStepTask" ),
		IoStepTask
	);
	taskQueue.AddTask( ioTasker );
}



int main( int argc, char **argv )
{
	// Get count of hardware threads
	unsigned int hardwareThreads = std::thread::hardware_concurrency();

	// Set the SignalHandler to handle abort,
	// terminate and interrupt signals
	signal( SIGABRT, SignalHandler );
	signal( SIGTERM, SignalHandler );
	signal( SIGINT,  SignalHandler );


	// Pass the event queue to the server
	server.SetEventQueue( &eventQueue );

	// Temporary network listener, for testing purposes
	auto networkListener = make_shared<NetworkListener>();
	eventDispatcher.AddEventListener( NETWORK_EVENT, networkListener );


	// Create the threads
	if( !GenerateWorkerThreads( hardwareThreads ) )
	{
		cerr << __FILE__ << ":" << __LINE__-2 << ":GenerateWorkerThreads() failed, exiting." << endl;
		return 1;
	}

	// Create the core tasks
	GenerateVitalTasks();


	// Try to start the server / open the listening socket
	cout << "Starting server..." << endl;
	try
	{
		server.Init( ioService, 22001 );
		server.Accept();
	}
	catch( std::exception &e )
	{
		cerr << "Failed to start: " << e.what() << endl;
		cout << "Press enter to quit." << endl;
		getc( stdin );
		return 1;
	}
	cout << "Server started! Port is " << 22001 << "." << endl;



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
	while( !stopServer );
	cout << "Main loop ended!" << endl;


	// Command worker threads to stop
	cout << "Commanding the threads to stop." << endl;

	threadPool.contextListMutex.lock();
	for( auto context  = threadPool.contexts.begin();
	          context != threadPool.contexts.end();
	          context++ )
	{
		(*context)->shouldStop = true;
	}
	threadPool.contextListMutex.unlock();


	// Wait for the thread pool to empty
	cout << "Waiting for the threads to stop." << endl;

	int maxThreads = ignoreLastThread ? 1 : 0;

	do
	{
		threadPool.CleanThreads();
		std::this_thread::yield();
	}
	while( threadPool.threads.size() > maxThreads );

	cout << "Threads stopped!" << endl;


	// Finish
	std::cout << endl << "Finished, press enter to quit." << std::endl;
	getc( stdin );

	return 0;
}

