#include "clientGameState.hh"

#include "network/serverConnection.hh"
#include "network/serializable.hh"
#include "world/objectEvents.hh"
#include "task.hh"
#include "logger.hh"
#include "threadPool.hh"

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
	eventDispatcher.AddEventListener( STATE_EVENT,   static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( OBJECT_EVENT,  static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( NETWORK_EVENT, static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( SDL_INPUT_EVENT,  static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( SDL_WINDOW_EVENT, static_cast<EventListenerPtr>( this ) );
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



void ClientGameState::Render()
{
	glClearColor( 0.5, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );
	//SDL_SetWindowBordered( sdlWindow, SDL_bool( 0 ) );

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


		default:
			LOG( "Undefined event sub type: '" << e->subType );
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
		LOG_ERROR( "Received invalid type!" );
		return;
	}


	EventSubType subType = static_cast<EventSubType>(
		UnserializeUint16( stream )
	);

	// Check the the sub type
	if( subType >= EVENT_SUB_TYPE_COUNT )
	{
		LOG_ERROR( "Received invalid sub type!" );
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
	// - It's a mess.
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
			LOG_ERROR( ToString( "Constructing event of type " <<
				                    (int)type << " not handled!"  ));
	}
}

