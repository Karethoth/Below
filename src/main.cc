#include <iostream>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <exception>
#include <boost/asio.hpp>

#include "logger.hh"

#include "threadPool.hh"

#include "events/eventQueue.hh"
#include "events/eventDispatcher.hh"
#include "network/networkEvents.hh"
#include "network/serverConnection.hh"

#include "world/entity.hh"
#include "managers/shaderProgramManager.hh"


#if _DEBUG || DEBUG
	#define DEBUG_MODE 1
#else
	#define DEBUG_MODE 0
#endif


#ifdef __WIN32__
	#pragma comment( lib, "opengl32.lib" )
	#pragma comment( lib, "SDL2.lib" )

	#if !DEBUG_MODE
		#pragma comment( lib, "glew32.lib" )

		// Let's disable the console
		#pragma comment( linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup" )

	#else
		#pragma comment( lib, "glew32d.lib" )
	#endif

	#undef main
#endif


using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;


// Some global queues, pools, etc.
ThreadPool      threadPool;
TaskQueue       taskQueue;
EventQueue      eventQueue;
EventDispatcher eventDispatcher;
io_service      ioService;

std::shared_ptr<ServerConnection> connection;

std::atomic<unsigned int> eventHandlerTasks;


// Managers
ShaderProgramManager shaderProgramManager;



// SDL and OpenGL globals
SDL_Window *sdlWindow = 0;
SDL_GLContext openglContext;


// Few global flags
bool sdlInitialized = false;
bool glInitialized  = false;
bool stopClient     = false;
bool windowFocus    = false;


// The worker loop, threads fetch tasks and execute them.
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
				LOG( "We connected!" );
				connection->Write( "Hello!" );
				break;

			case NETWORK_PART:
				LOG( "We disconnected!" );
				stopClient = true;
				break;

			case NETWORK_DATA_IN:
				dataIn = static_cast<DataInEvent*>( e );
				LOG( "Data in: '" << dataIn->data << "'" );
				connection->Write( dataIn->data );
				break;

			default:
				LOG( "Undefined event sub type: '" << e->subType );
		}
	}
};



// Task for handling an event
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



// Acknowledge SDL events, user input, etc.
void HandleSdlEvents()
{
	SDL_Event event;

	while( SDL_PollEvent( &event ) )
	{
		switch( event.type )
		{
			case SDL_KEYDOWN:
				break;

			case SDL_KEYUP:
				if( event.key.keysym.sym == SDLK_ESCAPE )
					stopClient = true;
				break;

			case SDL_QUIT:
				stopClient = true;
				break;

			case SDL_WINDOWEVENT:
				switch( event.window.event )
				{
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						windowFocus = true;
						break;

					case SDL_WINDOWEVENT_FOCUS_LOST:
						windowFocus = false;
						break;
				}
				break;
		}
	}
}



void Render()
{
	glClearColor( 0.5, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	SDL_GL_SwapWindow( sdlWindow );
}



bool InitSDL()
{
	// Handle the SDL stuff
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		LOG_ERROR( "SDL Error: " << SDL_GetError() );
		return false;
	};

	// Create the SDL2 window
	sdlWindow = SDL_CreateWindow(
		"Below Client", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 512, 512,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	if( !sdlWindow )
	{
		LOG_ERROR( "SDL Error: " << SDL_GetError() );
		return false;
	}


	// Create the icon for the window
	SDL_Surface *icon = SDL_LoadBMP( "data/icons/windowIcon.bmp" );
	if( !icon )
	{
		LOG_ERROR( "Warning: Icon couldn't be loaded!" );
	}
	else
	{
		// Set magenta as the transparent color
		unsigned int colorkey = SDL_MapRGB( icon->format, 255, 0, 255 );
		SDL_SetColorKey( icon, GL_SOURCE1_ALPHA, colorkey );

		// Set the icon
		SDL_SetWindowIcon( sdlWindow, icon );
		SDL_FreeSurface( icon );
	}

	return true;
}



bool InitGL()
{
	// Set the opengl context version
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );


	// Create the OpenGL context //
	openglContext = SDL_GL_CreateContext( sdlWindow );
	if( !openglContext )
	{
		LOG_ERROR( "SDL Error: " << SDL_GetError() );
		return false;
	}

	GLenum status = glewInit();
	if( status != GLEW_OK )
	{
		LOG_ERROR( "GLEW Error: " << glewGetErrorString( status ) );
		return false;
	}

	// VSync
	SDL_GL_SetSwapInterval( 1 );
	return true;
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



void Quit( int returnCode )
{
	// If we're connected, disconnect
	if( connection && connection->IsConnected() )
	{
		connection->Disconnect();
	}

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

	while( threadPool.threads.size() > 0 )
	{
		// Remove unjoinable threads
		threadPool.CleanThreads();

		std::this_thread::yield();
	}
	LOG( "Worker threads stopped!" );


	// Finish

	if( glInitialized )
	{
		SDL_GL_DeleteContext( openglContext );
	}

	if( sdlInitialized )
	{
		SDL_DestroyWindow( sdlWindow );
		SDL_Quit();
	}


#if DEBUG_MODE
	LOG( "Finished, press enter to quit." );
	getc( stdin );
#endif

	exit( returnCode );
}



int main( int argc, char *argv[] )
{
	// Get count of hardware threads
	unsigned int hardwareThreads = std::thread::hardware_concurrency();
	if( hardwareThreads <= 1 )
	{
		LOG( "Hardware supports just one real thread." );
		hardwareThreads = 2;
	}

	// Create the clock
	std::chrono::steady_clock clock;

	// Instantiate a network event listener
	auto networkListener = make_shared<NetworkListener>();
	eventDispatcher.AddEventListener( NETWORK_EVENT, networkListener );


	// Create the threads
	if( !GenerateWorkerThreads( hardwareThreads ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": GenerateWorkerThreads() failed, exiting." );

		Quit( 1 );
	}

	// Create the core tasks
	GenerateVitalTasks();


	// Init graphics
	if( !InitSDL() )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": InitSDL() failed, exiting." );
		Quit( 1 );
	}
	sdlInitialized = true;


	if( !InitGL() )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": InitGL() failed, exiting." );
		Quit( 1 );
	}
	glInitialized = true;


	//  Create task for connecting to the server:
	Task *connectTask = new Task();
	connectTask->name = "TryConnectingTask";
	connectTask->dependencies = 0;
	connectTask->f = []()
	{
		// Try to connect to the server
		LOG( "Connecting to the server..." );

		try
		{
			connection = make_shared<ServerConnection>( ioService, "localhost", 22001 );
			connection->SetEventQueue( &eventQueue );
			connection->Connect( ioService );
		}
		catch( std::exception &e )
		{
			LOG_ERROR( "Failed to connect: " << e.what() );
			stopClient = true;
		}

		if( connection->IsConnected() )
		{
			LOG( "Connected!" );
		}
	};
	taskQueue.AddTask( connectTask );


	// For timing in the main loop
	auto nextInfo = clock.now() + std::chrono::milliseconds( 500 );

	// Main loop
	LOG( "Starting the main loop" );

	do
	{
		// Do the main stuff
		HandleSdlEvents();
		Render();

		std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

		// And right away again, if it isn't the time for the info update
		if( clock.now() < nextInfo )
			continue;

		size_t taskCount  = taskQueue.GetTaskCount();
		size_t eventCount = eventQueue.GetEventCount();

		// Print out at least some stats
		LOG( "------------------------" <<  endl <<
		     "Task  queue size: " << taskCount << endl <<
		     "Event queue size: " << eventCount );

		std::this_thread::yield();
		nextInfo = clock.now( ) + std::chrono::milliseconds( 500 );
	}
	while( !stopClient );

	LOG( "Main loop ended!" );

	Quit( 0 );

	return 0;
}

