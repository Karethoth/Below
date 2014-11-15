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

	if( !objectManager.get() )
	{
		return;
	}

	lock_guard<mutex> objectManagerLock( objectManager->managerMutex );

	// Create the root node
	auto rootNode = std::make_shared<WorldNode>();
	objectManager->worldNodes.push_back( rootNode );


	// Create a test entity
	auto cubeEntity = make_shared<Entity>();
	cubeEntity->parent = rootNode->id;
	cubeEntity->material.color = { 1.0, 0.0, 1.0, 1.0 };
	cubeEntity->position = { 1.0, 0.0, 0.0 };
	cubeEntity->scale = { 0.5, 0.5, 0.5 };
	cubeEntity->UpdateModelMatrix();

	objectManager->entities.push_back( cubeEntity );
	objectManager->worldNodes.push_back( cubeEntity );
	rootNode->children.push_back( cubeEntity );

	auto cubeEntity2 = make_shared<Entity>();
	cubeEntity2->parent = cubeEntity->id;
	cubeEntity2->material.color = { 0.0, 1.0, 1.0, 1.0 };
	cubeEntity2->position = { 2.2, 0.0, 0.0 };
	cubeEntity2->scale = { 1.0, 1.0, 1.0 };
	cubeEntity2->UpdateModelMatrix();

	objectManager->entities.push_back( cubeEntity2 );
	objectManager->worldNodes.push_back( cubeEntity2 );
	cubeEntity->children.push_back( cubeEntity2 );
}



void ServerGameState::Destroy()
{
}



void ServerGameState::Tick( std::chrono::milliseconds deltaTime )
{
	static double cumulativeTime = 0.0;
	cumulativeTime += deltaTime.count() / 1000.0;

	// Update scene
	auto rotation = glm::rotate<float>(
		glm::mat4{},
		0.002f*deltaTime.count(),
		glm::normalize( glm::vec3{ 0.5f, 0.5f, 0.0f } )
	);

	auto rot = glm::toQuat( rotation );

	if( !objectManager.get() )
	{
		return;
	}


	// Stream to hold update message
	std::stringstream messageStream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);


	// Enter critical section
	objectManager->managerMutex.lock();

	// Update the scene
	if( objectManager->entities.size() >= 2 )
	{
		objectManager->entities[0]->position.Update( glm::vec3( sin( cumulativeTime*2 )*2, 0, 0 ) );
		objectManager->entities[0]->rotation.Update( objectManager->entities[0]->rotation.Get() * rot );
		objectManager->entities[1]->rotation.Update( objectManager->entities[1]->rotation.Get() * glm::inverse( rot ) );
	}

	// Calculate the model matrices for all entities
	// and generate an update event for each of them
	// to be broadcasted to the clients.
	for( auto& node : objectManager->worldNodes )
	{
		if( node->parent == 0 )
		{
			node->UpdateModelMatrix();
		}

		// Broadcast position and rotation of each node to clients
		std::stringstream stream(
			stringstream::in |
			stringstream::out |
			stringstream::binary
		);

		// Create packet body
		SerializeUint8( stream, (uint8_t)OBJECT_EVENT );
		SerializeUint16( stream, (uint16_t)OBJECT_UPDATE );
		SerializeUint32( stream, (uint32_t)node->id );
		stream << node->Serialize( { "position", "rotation" } );

		// Add packet length
		auto packet = stream.str();
		SerializeUint16( messageStream, packet.size()+2 );
		messageStream << packet;
	}

	// Leave critical section
	objectManager->managerMutex.unlock();


	// Send the update message to every client
	auto updateMessage = messageStream.str();

	{
		lock_guard<mutex> clientListLock( server.clientListMutex );
		for( auto &client : server.clientList )
		{
			client.second->Write( updateMessage );
		}
	}

	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}



void ServerGameState::SendScene( unsigned int clientId )
{
	auto client = server.GetClient( clientId );
	if( !client )
	{
		return;
	}

	if( !objectManager.get() )
	{
		return;
	}

	lock_guard<mutex> lock{ objectManager->managerMutex };

	std::stringstream messageStream(
		stringstream::in |
		stringstream::out |
		stringstream::binary
	);

	for( auto& node : objectManager->worldNodes )
	{
		std::stringstream stream(
			stringstream::in |
			stringstream::out |
			stringstream::binary
		);

		SerializeUint8( stream, (uint8_t)OBJECT_EVENT );	// Event type
		SerializeUint16( stream, (uint16_t)OBJECT_CREATE ); // Event sub type

		std::shared_ptr<Entity> entity;

		switch( node->type )
		{
			case WORLD_NODE_OBJECT_TYPE:
				SerializeUint8( stream, WORLD_NODE_OBJECT_TYPE );
				stream << node->Serialize(); // Serialize all
				break;

			case ENTITY_OBJECT_TYPE:
				SerializeUint8( stream, ENTITY_OBJECT_TYPE );
				entity = static_pointer_cast<Entity>( node );
				stream << entity->Serialize();
				break;

			default:
				continue;
		}

		// Add packet length
		auto packet = stream.str();
		SerializeUint16( messageStream, packet.size()+2 );
		messageStream << packet;
	}

	// Send hierarchy info
	for( auto& node : objectManager->worldNodes )
	{
		std::stringstream stream(
			stringstream::in |
			stringstream::out |
			stringstream::binary
		);

		SerializeUint8( stream, (uint8_t)OBJECT_EVENT );	    // Event type
		SerializeUint16( stream, (uint16_t)OBJECT_PARENT_ADD ); // Event sub type
		SerializeUint32( stream, node->id );
		SerializeUint32( stream, node->parent );

		// Add packet length
		auto packet = stream.str();
		SerializeUint16( messageStream, packet.size()+2 );
		messageStream << packet;
	}

	auto msg = messageStream.str();
	client->Write( messageStream.str() );
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
				server.CleanBadConnections();
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

	// Construct the event
	switch( type )
	{
		case OBJECT_EVENT:
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
		return false;
	}
	LOG( "Server started! Port is " << 22001 << "." );

	return true;
}

