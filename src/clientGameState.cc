#include "clientGameState.hh"

#include "network/serverConnection.hh"
#include "network/serializable.hh"
#include "world/objectEvents.hh"
#include "task.hh"
#include "logger.hh"
#include "threadPool.hh"
#include "managers/shaderProgramManager.hh"
#include "sdlEvents.hh"

#include "graphics/obj.hh"

#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

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

	SDL_SetWindowBordered( sdlWindow, SDL_bool( 0 ) );

	// Create the root node
	/*
	auto rootNode = std::make_shared<WorldNode>();
	worldNodes.push_back( rootNode );
	*/

	// Create a camera
	cam.viewMatrix = glm::lookAt(
		glm::vec3( 1, 3, -5 ),
		glm::vec3( 0, 0, 0 ),
		glm::vec3( 0, 1, 0 )
	);

	cam.position         = glm::vec3( 0, 5, 0 );
	cam.projectionMatrix = glm::perspective( 45.0f, 680.f/400.f, 0.1f, 100.0f );

	// Create a test mesh
	OBJ obj;
	string path( "data/objects/cube.obj" );
	obj.Load( path );

	auto cube = make_shared<Mesh>();
	cube->vertices  = obj.vertices;
	cube->normals   = obj.normals;
	cube->texCoords = obj.uvs;
	cube->GenerateBuffers();

	meshes.push_back( cube );


	// Fetch uniform indices
	colorUniform            = shaderProgramManager->Get( "defaultShader" )->GetUniform( "u_color" );
	viewMatrixUniform       = shaderProgramManager->Get( "defaultShader" )->GetUniform( "u_viewMatrix" );
	projectionMatrixUniform = shaderProgramManager->Get( "defaultShader" )->GetUniform( "u_projectionMatrix" );
	modelMatrixUniform      = shaderProgramManager->Get( "defaultShader" )->GetUniform( "u_modelMatrix" );
}



void ClientGameState::Destroy()
{
	// If we're connected, disconnect
	if( state.connected && connection->IsConnected() )
	{
		connection->Disconnect();
		state.connected = false;
	}

	// Free meshes
	for( auto& mesh : meshes )
	{
		mesh->FreeVbo();
	}
}



void ClientGameState::Tick( std::chrono::milliseconds deltaTime )
{
	static double cumulativeTime = 0.0;
	cumulativeTime += deltaTime.count() / 1000.0;

	// If we're not connected try to
	if( !state.connected && !state.tryingToConnect )
	{
		Connect();
		state.tryingToConnect = true;
	}

	// Update camera position
	/*
	cam.position = glm::vec3( sinf( cumulativeTime )*5,
	                          cam.position.y,
	                          cosf( cumulativeTime )*5 );

	cam.viewMatrix = glm::lookAt(
		cam.position,
		glm::vec3( 0, 0, 0 ),
		glm::vec3( 0, 1, 0 )
	);
	*/

	// Update transform matrices
	if( !objectManager.get() )
	{
		return;
	}

	objectManager->managerMutex.lock();
	for( auto& node : objectManager->worldNodes )
	{
		node->position.Calculate();
		node->rotation.Calculate();
	}

	for( auto& node : objectManager->worldNodes )
	{
		if( node->parent == 0 )
		{
			node->UpdateModelMatrix();
		}
	}
	objectManager->managerMutex.unlock();
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
			//stopClient = true;
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
	objectManager->managerMutex.lock();
	glEnable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glEnable( GL_CULL_FACE );

	auto totalMillis = chrono::duration_cast<chrono::milliseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() );
	auto offset = totalMillis.count()/1000.0;
	float color[3];
	color[0] = sin( offset );
	color[1] = sin( offset*1.5 );
	color[2] = sin( offset*1.7 );
	if( color[0] < 0 ) color[0] *= -1.0;
	if( color[1] < 0 ) color[1] *= -1.0;
	if( color[2] < 0 ) color[2] *= -1.0;

	Material mat = { {1.0, 0.0, 1.0, 1.0} };

	glClearColor( color[0], color[1], color[2], 1.0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUniformMatrix4fv( viewMatrixUniform, 1, GL_FALSE, &cam.viewMatrix[0][0] );
	glUniformMatrix4fv( projectionMatrixUniform, 1, GL_FALSE, &cam.projectionMatrix[0][0] );

	GLuint defaultShader = 0;
	if( shaderProgramManager.get() )
	{
		defaultShader = shaderProgramManager->Get( "defaultShader" )->Get();
		glUseProgram( defaultShader );

		glEnableVertexAttribArray( 0 );
		glEnableVertexAttribArray( 1 );
		glEnableVertexAttribArray( 2 );

		for( auto& entity : objectManager->entities )
		{
			glUniformMatrix4fv( modelMatrixUniform, 1, GL_FALSE, &entity->modelMatrix[0][0] );

			glUniform4fv( colorUniform, 1, &entity->material.color[0] );

			glDrawArrays( GL_TRIANGLES, 0, meshes[0]->vertices.size() );
		}

		glDisableVertexAttribArray( 2 );
		glDisableVertexAttribArray( 1 );
		glDisableVertexAttribArray( 0 );
	}

	SDL_GL_SwapWindow( sdlWindow );
	objectManager->managerMutex.unlock();
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

	if( e->type == NETWORK_EVENT )
	{
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

	else if( e->type == SDL_WINDOW_EVENT )
	{
		SdlWindowResizeEvent *resizeEvent;
		float ratio;

		switch( e->subType )
		{
			case SDL_WINDOW_RESIZE:
				LOG( "Resize Event" );
				resizeEvent = static_cast<SdlWindowResizeEvent*>( e );
				ratio = static_cast<float>( resizeEvent->width ) /
				        static_cast<float>( resizeEvent->height );

				cam.projectionMatrix = glm::perspective( 45.0f, ratio, 0.1f, 100.0f );
				break;

			default:
				goto _unhandled;
		}
	}
	else if( e->type == OBJECT_EVENT )
	{
		return;
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

	ObjectCreateEvent    *create;
	ObjectDestroyEvent   *destroy;
	ObjectUpdateEvent    *update;
	ObjectParentAddEvent *parentAdd;
	ObjectChildAddEvent  *childAdd;

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
					create->objectType = static_cast<WorldObjectType>( UnserializeUint8( stream ) );

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


				case( OBJECT_PARENT_ADD ):
					parentAdd = new ObjectParentAddEvent();
					parentAdd->type = OBJECT_EVENT;
					parentAdd->subType = OBJECT_PARENT_ADD;
					parentAdd->objectId = UnserializeUint32( stream );
					parentAdd->parentId  = UnserializeUint32( stream );
					eventQueue.AddEvent( parentAdd );
					break;


				case( OBJECT_CHILD_ADD ):
					childAdd = new ObjectChildAddEvent();
					childAdd->type = OBJECT_EVENT;
					childAdd->subType = OBJECT_CHILD_ADD;
					childAdd->objectId = UnserializeUint32( stream );
					childAdd->childId  = UnserializeUint32( stream );
					eventQueue.AddEvent( childAdd );
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

