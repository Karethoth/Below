#ifdef _MSC_VER
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

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
#include "world/objectEvents.hh"
#include "sdlEvents.hh"

#include "network/serverConnection.hh"

#include "world/entity.hh"
#include "managers/shaderProgramManager.hh"
#include "managers/clientObjectManager.hh"

#include "clientGameState.hh"
#include "graphics/obj.hh"


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
ClientGameState gameState;
ThreadPool      threadPool;
TaskQueue       taskQueue;
EventQueue      eventQueue;
EventDispatcher eventDispatcher;
io_service      ioService;

std::atomic<unsigned int> eventHandlerTasks;


// Managers
std::shared_ptr<ShaderProgramManager> shaderProgramManager;
std::shared_ptr<ClientObjectManager>  objectManager;


// SDL and OpenGL globals
SDL_Window *sdlWindow = nullptr;
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

		// Mark task as finished
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



static auto lastSceneUpdate = chrono::steady_clock::now();
void SceneUpdateTask()
{

	// Calculate the delta time
	auto now = chrono::steady_clock::now();
	chrono::milliseconds deltaTime = chrono::duration_cast<chrono::milliseconds>( (now - lastSceneUpdate) );
	lastSceneUpdate = now;

	// Tick
	gameState.Tick( deltaTime );
	this_thread::sleep_until( now + chrono::milliseconds( 10 ) );

	// Generate the next task
	Task *nextTick = new Task(
		std::string( "SceneUpdateTask" ),
		SceneUpdateTask
	);

	taskQueue.AddTask( nextTick );
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
		SDL_WINDOWPOS_CENTERED, 680, 400,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	if( !sdlWindow )
	{
		LOG_ERROR( "SDL Error: " << SDL_GetError() );
		return false;
	}

	// Set maximum and minimum sizes:
	SDL_SetWindowMaximumSize( sdlWindow, 2800, 2000 );
	SDL_SetWindowMinimumSize( sdlWindow, 460, 380 );

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
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );


	// Create the OpenGL context //
	openglContext = SDL_GL_CreateContext( sdlWindow );
	if( !openglContext )
	{
		LOG_ERROR( "SDL Error: " << SDL_GetError() );
		return false;
	}

	glewExperimental = GL_TRUE;
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



// Acknowledge SDL events, user input, etc.
// Transforms the event to a custom struct
void HandleSdlEvents()
{
	SDL_Event event;
	SdlMouseButtonEvent *mouseButton;
	SdlMouseMoveEvent   *mouseMove;
	SdlMouseWheelEvent  *mouseWheel;
	SdlTextInputEvent   *textInput;
	SdlTextEditingEvent *textEditing;
	SdlWindowResizeEvent      *windowResize;
	SdlWindowFocusChangeEvent *windowFocusChange;

	while( SDL_PollEvent( &event ) )
	{
		switch( event.type )
		{
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouseButton = new SdlMouseButtonEvent();

				if( event.button.state == SDL_PRESSED )
				{
					mouseButton->subType = SDL_MOUSE_DOWN;
				}
				else
				{
					mouseButton->subType = SDL_MOUSE_UP;
				}

				mouseButton->button = event.button;
				eventQueue.AddEvent( mouseButton );
				break;


			case SDL_MOUSEMOTION:
				mouseMove = new SdlMouseMoveEvent();
				mouseMove->motion = event.motion;
				eventQueue.AddEvent( mouseMove );
				break;


			case SDL_MOUSEWHEEL:
				mouseWheel = new SdlMouseWheelEvent();
				mouseWheel->wheel = event.wheel;
				eventQueue.AddEvent( mouseWheel );



			case SDL_TEXTINPUT:
				textInput = new SdlTextInputEvent();
				textInput->text = std::string( event.text.text );
				eventQueue.AddEvent( textInput );
				break;


			case SDL_TEXTEDITING:
				textEditing = new SdlTextEditingEvent();
				textEditing->composition     = std::string( event.edit.text );
				textEditing->cursor          = event.edit.start;
				textEditing->selectionLength = event.edit.length;
				eventQueue.AddEvent( static_cast<Event*>( textEditing ) );
				break;


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
						windowFocusChange = new SdlWindowFocusChangeEvent();
						windowFocusChange->focusGained = true;
						eventQueue.AddEvent( windowFocusChange );
						break;


					case SDL_WINDOWEVENT_FOCUS_LOST:
						windowFocusChange = new SdlWindowFocusChangeEvent();
						windowFocusChange->focusGained = false;
						eventQueue.AddEvent( windowFocusChange );
						break;


					case SDL_WINDOWEVENT_RESIZED:
						LOG( ToString(
							"New window size: " <<
							event.window.data1  <<
							"x" << event.window.data2
						) );

						windowResize = new SdlWindowResizeEvent();
						windowResize->width  = event.window.data1;
						windowResize->height = event.window.data2;
						glViewport( 0, 0, windowResize->width, windowResize->height );
						eventQueue.AddEvent( windowResize );
						break;


					default:
						break;
				}
				break;
		}
	}
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



void GenerateUpdateTask()
{
	// Create scene/physics update task
	Task *nextTick = new Task(
		std::string( "SceneUpdateTask" ),
		SceneUpdateTask
	);

	taskQueue.AddTask( nextTick );
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

	if( noExit )
	{
		return;
	}

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

	// Instantiate an object manager and add it as an object event listener
	objectManager = make_shared<ClientObjectManager>();
	eventDispatcher.AddEventListener( OBJECT_EVENT, objectManager );


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

	// Load shaders
	shaderProgramManager = make_shared<ShaderProgramManager>();

	// Default Shader
	map<string, GLuint> defaultProgramAttributes;
	defaultProgramAttributes["vertexPosition"] = 0;
	defaultProgramAttributes["vertexNormal"]   = 1;
	defaultProgramAttributes["vertexUV"]       = 2;

	if( !shaderProgramManager->Load( "data/shaders/defaultShader", "defaultShader", defaultProgramAttributes ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": Generating default shader failed, exiting." );
		Quit( 1 );
	}

	// GUI Shader
	map<string, GLuint> guiProgramAttributes;
	guiProgramAttributes["vertexPosition"] = 0;
	guiProgramAttributes["vertexNormal"]   = 1;
	guiProgramAttributes["vertexUV"]       = 2;

	if( !shaderProgramManager->Load( "data/shaders/guiShader", "guiShader", guiProgramAttributes ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": Generating GUI shader failed, exiting." );
		Quit( 1 );
	}

	// Create the game state
	gameState.objectManager = objectManager;
	gameState.Create();

	// Set minimum frame time
	const auto frameMinLength = chrono::milliseconds{ 1000/60 };

	// Start update loop
	GenerateUpdateTask();

	// Main loop
	LOG( "Starting the main loop" );

	while( !stopClient )
	{
		// Calculate the delta time
		auto frameStart = chrono::steady_clock::now();

		// Do the main stuff
		HandleSdlEvents();

		// Update the current game state
		gameState.Render();

		this_thread::sleep_until( frameStart + frameMinLength );
	}

	LOG( "Main loop ended!" );

	Quit( 0, true );
	gameState.Destroy();

	return 0;
}

