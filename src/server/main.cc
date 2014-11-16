#include "../logger.hh"

#include "../threadPool.hh"
#include "../network/server.hh"
#include "../events/eventQueue.hh"
#include "../events/eventDispatcher.hh"

#include "serverGameState.hh"

#include <iostream>
#include <thread>
#include <vector>
#include <exception>
#include <csignal>

#include <boost/asio.hpp>


using namespace std;
using boost::asio::ip::tcp;


// Some global queues, pools, etc.
ThreadPool      threadPool;
TaskQueue       taskQueue;
EventQueue      eventQueue;
EventDispatcher eventDispatcher;
boost::asio::io_service ioService;

std::atomic<unsigned int> eventHandlerTasks;

// Managers
std::shared_ptr<ServerObjectManager>  objectManager;

// Server stopper
bool stopServer = false;
bool ignoreLastThread = false;


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
				join = static_cast<JoinEvent*>( e );
				LOG( "Client " << join->clientId << " joined!" );
				break;

			case NETWORK_PART:
				part = static_cast<PartEvent*>( e );
				LOG( "Client " << part->clientId << " parted!" );
				break;

			case NETWORK_DATA_IN:
				dataIn = static_cast<DataInEvent*>( e );
				LOG( "Data in from client " << dataIn->clientId << ": '" << dataIn->data << "'" );
				//server.GetClient( dataIn->clientId )->Write( dataIn->data );
				break;

			default:
				LOG( "Undefined event sub type: '" << static_cast<EVENT_SUB_TYPE>( e->subType ) );
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

	eventHandlerTasks--;

	// Free the event
	delete e;
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
		taskQueue.AddTask( eventTask );

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
	LOG( "Creating worker threads" );

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
		LOG( "Created thread: " << newThread->get_id() );
		threadPool.threads.push_back( newThread );
		threadPool.threadListMutex.unlock();
	}

	return true;
}



void GenerateVitalTasks()
{
	// Create a task to generate tasks to handle events
	LOG( "Creating the event handler generator." );

	Task *eventTasker = new Task(
		std::string( "EventHandlerGenerator" ),
		EventHandlerGenerator
	);
	taskQueue.AddTask( eventTasker );


	// Create a task to run the network io services
	LOG( "Creating the network I/O tasker." );

	Task *ioTasker = new Task(
		std::string( "IoStepTask" ),
		IoStepTask
	);
	taskQueue.AddTask( ioTasker );
}



void Quit( int returnCode, bool noExit=false )
{
	// Command worker threads to stop
	LOG( "Stopping the worker threads!" );

	threadPool.contextListMutex.lock();
	for( auto context  = threadPool.contexts.begin();
	          context != threadPool.contexts.end();
	          context++ )
	{
		(*context)->shouldStop = true;
	}
	threadPool.contextListMutex.unlock();


	// Wait for the thread pool to empty
	LOG( "Worker threads have been commanded to stop." );

	// Remove unjoinable threads
	threadPool.CleanThreads();

	// Give threads few seconds of time to close themselves,
	// we don't want to hang if a thread is blocking.
	this_thread::sleep_for( chrono::seconds( 2 ) );

	LOG( "Worker threads stopped!" );


	// Finish

#if DEBUG_MODE
	LOG( "Finished, press enter to quit." );
	getc( stdin );
#endif

	if( noExit )
	{
		return;
	}

	exit( returnCode );
}



int main( int argc, char **argv )
{
	// Get count of hardware threads
	unsigned int hardwareThreads = std::thread::hardware_concurrency();
	if( hardwareThreads <= 1 )
	{
		LOG( "Hardware supports just one real thread." );
		hardwareThreads = 2;
	}

	// Set the SignalHandler to handle abort,
	// terminate and interrupt signals
	signal( SIGABRT, SignalHandler );
	signal( SIGTERM, SignalHandler );
	signal( SIGINT,  SignalHandler );


	// Pass the event queue to the server
	auto gameState = std::make_shared<ServerGameState>();
	gameState->server.SetEventQueue( &eventQueue );

	// Create object manager
	objectManager = make_shared<ServerObjectManager>();
	gameState->objectManager = objectManager;

	// Temporary network listener, for testing purposes
	eventDispatcher.AddEventListener( NETWORK_EVENT, gameState );


	// Create the threads
	if( !GenerateWorkerThreads( hardwareThreads ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": GenerateWorkerThreads() failed, exiting." );
		return 1;
	}

	// Create the core tasks
	GenerateVitalTasks();

	

	// For timing in the main loop
	auto now        = chrono::steady_clock::now();
	auto lastUpdate = chrono::steady_clock::now();


	// Create the game state
	gameState->Create();

	if( !gameState->StartServer() )
	{
		stopServer = true;
	}

	// Main loop
	LOG( "Starting the main loop" );

	do
	{
		// Calculate the delta time
		now = chrono::steady_clock::now();
		chrono::milliseconds deltaTime = chrono::duration_cast<chrono::milliseconds>( (now - lastUpdate) );
		lastUpdate = now;

		// Update the current game state
		gameState->Tick( deltaTime );
	}
	while( !stopServer );

	LOG( "Main loop ended!" );

	Quit( 0, true );
	gameState->Destroy();


	// Finish
	LOG( "Finished, press enter to quit." );
	getc( stdin );

	return 0;
}

