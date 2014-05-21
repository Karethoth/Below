#include <iostream>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <SDL_Main.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <exception>
#include <boost\asio.hpp>

#include "threadPool.hh"

#include "events/eventQueue.hh"
#include "events/eventDispatcher.hh"
#include "network/networkEvents.hh"
#include "network/serverConnection.hh"

#include "world/entity.hh"


#ifdef __WIN32__
	#pragma comment( lib, "opengl32.lib" )
	#pragma comment( lib, "SDL2.lib" )
	//#pragma comment( lib, "SDL2main.lib" )

	#ifndef _DEBUG
		#define DEBUG_MODE false
		#pragma comment( lib, "glew32.lib" )

		// Let's disable the console
		#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

	#else
		#define DEBUG_MODE true
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

std::atomic<unsigned int> eventHandlerTasks = 0;


// SDL and OpenGL globals
SDL_Window *sdlWindow = 0;
SDL_GLContext openglContext;

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
		if( task->f )
		{
			task->f();
		}

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
				cout << "We joined!" << endl;
				break;

			case NETWORK_PART:
				cout << "We disconnected!" << endl;
				stopClient = true;
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

	// Free the event
	delete e;

	eventHandlerTasks--;
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



// The generator of Event handler tasks
void EventHandlerGenerator()
{
	while( eventQueue.GetEventCount() > eventHandlerTasks )
	{
		Task *eventTask = new Task();
		eventTask->dependencies = 0;
		eventTask->f = EventHandlerTask;
		taskQueue.AddTask( eventTask );
		eventHandlerTasks++;
	}

	Task *eventTasker = new Task();
	eventTasker->dependencies = 0;
	eventTasker->f = EventHandlerGenerator;
	taskQueue.AddTask( eventTasker );
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
		}
	}
}



void Render()
{
	glClearColor( 0.5, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	SDL_GL_SwapWindow( sdlWindow );
}



int main( int argc, char *argv[] )
{
	// Get count of hardware threads
	unsigned int hardwareThreads = std::thread::hardware_concurrency();

	// Create the clock
	std::chrono::steady_clock clock;

	auto networkListener = make_shared<NetworkListener>();
	eventDispatcher.AddEventListener( NETWORK_EVENT, networkListener );

	SDL_Init( SDL_INIT_EVERYTHING );


	// Set the opengl context version
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );


	// Create the SDL2 window
	sdlWindow = SDL_CreateWindow(
		"Below Client", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 512, 512,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);


	// Create the icon for the window
	SDL_Surface *icon = SDL_LoadBMP( "../data/icons/windowIcon.bmp" );
	if( !icon )
	{
		cerr << "Icon couldn't be loaded!" << endl;
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


	// Create the OpenGL context
	openglContext = SDL_GL_CreateContext(sdlWindow);

	GLenum status = glewInit();
	if( status != GLEW_OK )
	{
		std::cerr << "GLEW Error: " << glewGetErrorString( status ) << "\n";
		return 1;
	}

	// VSync
	SDL_GL_SetSwapInterval( 1 );


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
	cout << "Creating the event handler generator." << endl;

	Task *eventTasker = new Task();
	eventTasker->dependencies = 0;
	eventTasker->f = EventHandlerGenerator;
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


	// Try to connect to the server
	cout << "Connecting to the server..." << endl;
	try
	{
		connection = make_shared<ServerConnection>( ioService, "localhost", 22001 );
		connection->SetEventQueue( &eventQueue );
		connection->Connect( ioService );
	}
	catch( std::exception &e )
	{
		cerr << "Failed to connect: " << e.what() << endl;
		stopClient = true;
	}

	if( connection->IsConnected() )
	{
		cout << "Connected!" << endl;
	}


	// For timing in the main loop
	auto nextInfo = clock.now() + std::chrono::milliseconds( 500 );

	// Main loop
	cout << "Starting the main loop" << endl;

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
		cout << "------------------------" <<  endl;
		cout << "Task  queue size: " << taskCount  << endl;
		cout << "Event queue size: " << eventCount << endl;

		std::this_thread::yield();
		nextInfo = clock.now( ) + std::chrono::milliseconds( 500 );
	}
	while( !stopClient );

	cout << endl << "Main loop ended!" << endl;

	// If we're connected, disconnect
	if( connection->IsConnected() )
	{
		connection->Disconnect();
	}

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
	cout << "Cleaning contexts." << endl;

	SDL_GL_DeleteContext( openglContext );
	SDL_DestroyWindow( sdlWindow );
	SDL_Quit();

	std::cout << "Finished, press enter to quit." << std::endl;

	getc( stdin );
	return 0;
}

