#include "serverGameState.hh"

#include "../network/serverConnection.hh"
#include "../network/serializable.hh"
#include "../world/objectEvents.hh"
#include "../task.hh"
#include "../logger.hh"
#include "../threadPool.hh"
#include "../managers/shaderProgramManager.hh"

#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;


extern bool stopServer;

extern ThreadPool      threadPool;
extern TaskQueue       taskQueue;
extern EventQueue      eventQueue;
extern EventDispatcher eventDispatcher;
extern io_service      ioService;

extern std::shared_ptr<ShaderProgramManager> shaderProgramManager;

std::shared_ptr<ServerConnection> connection;



ServerGameState::ServerGameState()
{
}



ServerGameState::~ServerGameState()
{
}



void ServerGameState::Create()
{
	// Clear conflicting event listeners
	eventDispatcher.eventListeners[STATE_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[STATE_EVENT]->collection.clear();
	eventDispatcher.eventListeners[STATE_EVENT]->collectionMutex.unlock();

	eventDispatcher.eventListeners[NETWORK_EVENT]->collectionMutex.lock();
	eventDispatcher.eventListeners[NETWORK_EVENT]->collection.clear();
	eventDispatcher.eventListeners[NETWORK_EVENT]->collectionMutex.unlock();


	// Set this as the new event listener for these event categories
	eventDispatcher.AddEventListener( STATE_EVENT,      static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( OBJECT_EVENT,     static_cast<EventListenerPtr>( this ) );
	eventDispatcher.AddEventListener( NETWORK_EVENT,    static_cast<EventListenerPtr>( this ) );

	// Create the root node
	auto rootNode = std::make_shared<WorldNode>();
	worldNodes.push_back( rootNode );


	// Create a test entity
	auto cubeEntity = make_shared<Entity>();
	cubeEntity->parent = rootNode->id;
	cubeEntity->meshId = 0;
	cubeEntity->textureId = 0;
	cubeEntity->material.color = { 1.0, 0.0, 0.0, 1.0 };
	cubeEntity->position = { 1.0, 0.0, 0.0 };
	cubeEntity->scale = { 0.5, 0.5, 0.5 };
	cubeEntity->UpdateModelMatrix();

	entities.push_back( cubeEntity );
	worldNodes.push_back( cubeEntity );
	rootNode->children.push_back( cubeEntity );

	auto cubeEntity2 = make_shared<Entity>();
	cubeEntity2->parent = rootNode->id;
	cubeEntity2->meshId = 0;
	cubeEntity2->textureId = 0;
	cubeEntity2->material.color = { 0.0, 0.0, 1.0, 1.0 };
	cubeEntity2->position = { 2.0, 0.0, 0.0 };
	cubeEntity2->scale = { 1.0, 1.0, 1.0 };
	cubeEntity2->UpdateModelMatrix();

	entities.push_back( cubeEntity2 );
	worldNodes.push_back( cubeEntity2 );
	cubeEntity->children.push_back( cubeEntity2 );
}



void ServerGameState::Destroy()
{
	entities.clear();
	worldNodes.clear();
}



void ServerGameState::Tick( std::chrono::milliseconds deltaTime )
{
	static double cumulativeTime = 0.0;
	cumulativeTime += deltaTime.count() / 1000.0;

	auto rot = glm::toQuat( glm::rotate<float>(
		glm::mat4{ 1.0 },
		0.002*deltaTime.count(),
		glm::vec3{ 0.5, 0.5, 0.1 }
	) );

	entities[0]->rotation *= rot;
	entities[1]->rotation *= glm::inverse( rot*rot );

	for( auto& node : worldNodes )
	{
		if( node->parent == 0 )
		{
			node->UpdateModelMatrix();
		}
	}

	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}



void ServerGameState::SendScene( unsigned int clientId )
{
	auto client = server.GetClient( clientId );
	if( !client )
	{
		return;
	}

	for( auto& node : worldNodes )
	{
		std::stringstream stream(
			stringstream::in |
			stringstream::out |
			stringstream::binary
		);

		SerializeUint8( stream, (uint8_t)OBJECT_EVENT );	// Event type
		SerializeUint16( stream, (uint16_t)OBJECT_CREATE ); // Event sub type
		SerializeUint8( stream, WORLD_NODE_OBJECT_TYPE );   // Object type

		std::shared_ptr<Entity> entity;

		switch( node->type )
		{
			case WORLD_NODE_OBJECT_TYPE:
				stream << node->Serialize( vector<string>() );    // Serialize all
				stream << node->Serialize();
				break;

			case ENTITY_OBJECT_TYPE:
				entity = static_pointer_cast<Entity>( node );
				stream << entity->Serialize();
				break;

			default:
				continue;
		}

		client->Write( stream.str() );
	}
}



void ServerGameState::HandleEvent( Event *e )
{
	JoinEvent   *join;
	PartEvent   *part;
	DataInEvent *dataIn;

	// Stream for manual package creation
	std::stringstream stream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);

	if( e->type == NETWORK_EVENT )
	{
		switch( e->subType )
		{
			case NETWORK_JOIN:
				join = static_cast<JoinEvent*>( e );
				LOG( "Client " << join->clientId << " joined!" );
				SendScene( join->clientId );
				break;

			case NETWORK_PART:
				part = static_cast<PartEvent*>( e );
				LOG( "Client " << part->clientId << " parted!" );
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
				goto _unhandled;
		}
	}
	else
	{
		goto _unhandled;
	}

	return;


_unhandled:
	LOG( "Unhandled event : '"
		<< EventTypeToStr( e->type )
		<< " - "
		<< EventSubTypeToStr( e->subType ) );
}



void ServerGameState::HandleDataInEvent( DataInEvent *e )
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



bool ServerGameState::StartServer()
{
	// Try to start the server / open the listening socket
	LOG( "Starting server..." );
	try
	{
		server.Init( ioService, 22001 );
		server.Accept();
	}
	catch( std::exception &e )
	{
		LOG_ERROR( "Failed to start: " << e.what() );
		LOG( "Press enter to quit." );
		getc( stdin );
		return 1;
	}
	LOG( "Server started! Port is " << 22001 << "." );
}
