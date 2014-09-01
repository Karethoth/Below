#include "clientGameState.hh"

#include "network/serverConnection.hh"
#include "network/serializable.hh"
#include "world/objectEvents.hh"
#include "task.hh"
#include "logger.hh"
#include "threadPool.hh"
#include "managers/shaderProgramManager.hh"

#include <thread>
#include <chrono>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;


extern bool stopClient;

extern ThreadPool      threadPool;
extern TaskQueue       taskQueue;
extern EventQueue      eventQueue;
extern EventDispatcher eventDispatcher;
extern io_service      ioService;

extern SDL_Window *sdlWindow;

extern std::shared_ptr<ShaderProgramManager> shaderProgramManager;

std::shared_ptr<ServerConnection> connection;



ClientGameState::ClientGameState()
{
	state.connected       = false;
	state.tryingToConnect = false;
}



ClientGameState::~ClientGameState()
{
	// If we're -still- connected, disconnect
	if( connection && connection->IsConnected() )
	{
		connection->Disconnect();
		state.connected = false;
	}
}


static GLuint VertexArrayID;
static GLuint vertexbuffer;
static GLint colorUniform;
static GLint scaleUniform;
static GLint positionUniform;
static const GLfloat g_vertex_buffer_data[] = {
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};

void ClientGameState::Create()
{
	// Clear conflicting event listeners
	eventDispatcher.eventListeners[STATE_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[STATE_EVENT]->collection.clear();
	eventDispatcher.eventListeners[STATE_EVENT]->collectionMutex.unlock();

	eventDispatcher.eventListeners[NETWORK_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[NETWORK_EVENT]->collection.clear();
	eventDispatcher.eventListeners[NETWORK_EVENT]->collectionMutex.unlock();

	eventDispatcher.eventListeners[SDL_INPUT_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[SDL_INPUT_EVENT]->collection.clear();
	eventDispatcher.eventListeners[SDL_INPUT_EVENT]->collectionMutex.unlock();

	eventDispatcher.eventListeners[SDL_WINDOW_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[SDL_WINDOW_EVENT]->collection.clear();
	eventDispatcher.eventListeners[SDL_WINDOW_EVENT]->collectionMutex.unlock();


	// Set this as the new event listener for these event categories
	eventDispatcher.AddEventListener( STATE_EVENT,      static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( OBJECT_EVENT,     static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( NETWORK_EVENT,    static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( SDL_INPUT_EVENT,  static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( SDL_WINDOW_EVENT, static_cast<EventListenerPtr>( this ) );

	//SDL_SetWindowBordered( sdlWindow, SDL_bool( 0 ) );

	// Create test VAO
	glGenVertexArrays( 1, &VertexArrayID );
	glBindVertexArray( VertexArrayID );
	glGenBuffers( 1, &vertexbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW );

	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glVertexAttribPointer(
		shaderProgramManager->Get( "guiShader" )->GetAttribute( "vertexPosition" ), // Index
		3,        // Size
		GL_FLOAT, // Type
		GL_FALSE, // Normalized?
		0,        // Stride
		(void*)0  // Offset
	);

	glDisableVertexAttribArray( 0 );

	// Fetch uniform indices
	colorUniform    = shaderProgramManager->Get( "guiShader" )->GetUniform( "u_color" );
	scaleUniform    = shaderProgramManager->Get( "guiShader" )->GetUniform( "u_scale" );
	positionUniform = shaderProgramManager->Get( "guiShader" )->GetUniform( "u_position" );
}



void ClientGameState::Destroy()
{
	// If we're connected, disconnect
	if( state.connected && connection->IsConnected() )
	{
		connection->Disconnect();
		state.connected = false;
	}
}



void ClientGameState::Tick( std::chrono::milliseconds deltaTime )
{
	// If we're not connected try to
	if( !state.connected && !state.tryingToConnect )
	{
		Connect();
		state.tryingToConnect = true;
	}

	// Render
	Render();

	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}



void ClientGameState::Connect()
{
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
}

auto startTime = std::chrono::high_resolution_clock::now();

void ClientGameState::Render()
{
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if( std::chrono::high_resolution_clock::now() < startTime )
	{
		startTime = std::chrono::high_resolution_clock::now();
	}

	auto offset = (std::chrono::high_resolution_clock::now()-startTime).count()/10000.0;
	float color[3];
	color[0] = sin( offset/1000.0 );
	color[1] = sin( offset/2000.0 );
	color[2] = sin( offset/500.0 );
	if( color[0] < 0 ) color[0] *= -1.0;
	if( color[1] < 0 ) color[1] *= -1.0;
	if( color[2] < 0 ) color[2] *= -1.0;

	glClearColor( color[0], color[1], color[2], 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	GLuint guiShader = 0;
	if( shaderProgramManager.get() )
	{
		guiShader = shaderProgramManager->Get( "guiShader" )->Get();
		glUseProgram( guiShader );

		glEnableVertexAttribArray( 0 );

		for( int y=0; y < 4; y++ )
		{
			for( int x=0; x < 4; x++ )
			{
				glUniform4f( colorUniform, 0.25*(y), 0.0, 0.25*(y), 0.25*(x+1) );
				glUniform2f( positionUniform, 0.25*x, 0.25*(y) );
				glUniform2f( scaleUniform, 0.5, 0.5 );
				glDrawArrays( GL_TRIANGLES, 0, 6 );
			}
		}

		glDisableVertexAttribArray( 0 );
	}

	SDL_GL_SwapWindow( sdlWindow );
}



void ClientGameState::HandleEvent( Event *e )
{
	DataInEvent *dataIn;

	// Stream for manual package creation
	std::stringstream stream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);


	switch( e->subType )
	{
		case NETWORK_JOIN:
			LOG( "We connected!" );

			// Update state
			state.connected       = true;
			state.tryingToConnect = false;

			// For now, construct few events here
			// manually and send them to the server:

			// Create an object of type 1
			//SerializeUint8( stream, (uint8_t)OBJECT_EVENT );	// Event type
			//SerializeUint16( stream, (uint16_t)OBJECT_CREATE ); // Event sub type
			//SerializeUint8( stream, (uint8_t)1 );               // Object type
			//stream << tmpNode.Serialize( vector<string>() );    // Serialize all
			break;


		case NETWORK_PART:
			LOG( "We disconnected!" );
			state.connected = false;
			stopClient      = true;
			break;


		case NETWORK_DATA_IN:
			dataIn = static_cast<DataInEvent*>( e );
			LOG( "Data in: '" << dataIn->data << "'" );
			HandleDataInEvent( dataIn );
			break;


		case NETWORK_PING:
			LOG( "Ping" );
			break;


		case NETWORK_PONG:
			LOG( "Pong" );
			break;


		default:
			LOG( "Unhandled event : '"
			     << EventTypeToStr( e->type )
			     << " - "
			     << EventSubTypeToStr( e->subType ) );
	}
}



void ClientGameState::HandleDataInEvent( DataInEvent *e )
{
	char buffer[USHRT_MAX];

	stringstream stream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);
	stream << e->data;


	EventType type = static_cast<EventType>(
		UnserializeUint8( stream )
	);

	// Check the type
	if( type >= EVENT_TYPE_COUNT )
	{
		LOG_ERROR( "Received invalid type(" << static_cast<unsigned int>( type ) << ")!" );
		return;
	}


	EventSubType subType = static_cast<EventSubType>(
		UnserializeUint16( stream )
	);

	// Check the the sub type
	if( subType >= EVENT_SUB_TYPE_COUNT )
	{
		LOG_ERROR( "Received invalid sub type(" << static_cast<unsigned int>( subType ) << ")!" );
		return;
	}

	// Copy the data to the dataStream
	stringstream dataStream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);

	ObjectCreateEvent  *create;
	ObjectDestroyEvent *destroy;
	ObjectUpdateEvent  *update;

	size_t dataCount;

	// Construct the event
	switch( type )
	{
		case OBJECT_EVENT:
			switch( subType )
			{
				case( OBJECT_CREATE ):
					create = new ObjectCreateEvent();
					create->type = OBJECT_EVENT;
					create->subType = OBJECT_CREATE;
					create->objectType = UnserializeUint8( stream );

					dataCount = stream.str().length() - 4;
					stream.read( buffer, dataCount );
					dataStream.write( buffer, dataCount );

					create->data = dataStream.str();
					eventQueue.AddEvent( create );
					break;


				case( OBJECT_DESTROY ):
					destroy = new ObjectDestroyEvent();
					destroy->type = OBJECT_EVENT;
					destroy->subType = OBJECT_DESTROY;
					destroy->objectId = UnserializeUint32( stream );
					eventQueue.AddEvent( destroy );
					break;


				case( OBJECT_UPDATE ):
					update = new ObjectUpdateEvent();
					update->type = OBJECT_EVENT;
					update->subType = OBJECT_UPDATE;
					update->objectId = UnserializeUint32( stream );

					dataCount = stream.str().length() - 7;
					stream.read( buffer, dataCount );
					dataStream.write( buffer, dataCount );

					update->data = dataStream.str();
					eventQueue.AddEvent( update );
					break;


				default:
					break;
			}
			break;


		default:
			LOG_ERROR( ToString( "Constructing event of type "
			                     << EventTypeToStr( type )
			                     << " not yet handled!" ) );
	}
}

