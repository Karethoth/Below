#include "clientObjectManager.hh"
#include "../logger.hh"
#include "../world/objectEvents.hh"
#include "../world/entity.hh"

using namespace std;


void ClientObjectManager::HandleEvent( Event *e )
{
	lock_guard<std::mutex> lock( managerMutex );

	ObjectCreateEvent  *createEvent;
	ObjectDestroyEvent *destroyEvent;
	ObjectUpdateEvent  *updateEvent;

	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ServerObjectManager::HandleEvent." );
		return;
	}

	shared_ptr<WorldNode> newNode;
	shared_ptr<Entity>    newEntity;

	switch( e->subType )
	{
		case OBJECT_CREATE:
			createEvent = static_cast<ObjectCreateEvent*>( e );
			LOG( "Object creation started.." );

			if( createEvent->objectType == WORLD_NODE_OBJECT_TYPE )
			{
				newNode = make_shared<WorldNode>();
				newNode->Unserialize( createEvent->data );
			}
			else if( createEvent->objectType == ENTITY_OBJECT_TYPE )
			{
				newEntity = make_shared<Entity>();
				newEntity->Unserialize( createEvent->data );
				newNode = static_pointer_cast<WorldNode>( newEntity );
				entities.push_back( newEntity );
			}
			else
			{
				break;
			}

			LOG( "Object created!" );
			worldObjects.push_back( newNode );
			break;


		case OBJECT_DESTROY:
			destroyEvent = static_cast<ObjectDestroyEvent*>( e );
			//delete newNode;
			LOG( "Object destroyed!" );
			break;


		case OBJECT_UPDATE:
			updateEvent = static_cast<ObjectUpdateEvent*>( e );
			if( newNode )
			{
				newNode->Unserialize( updateEvent->data );
			}
			LOG( "Object updated!" );
			break;


		default:
			LOG( "Undefined event sub type: '" << static_cast<EVENT_SUB_TYPE>( e->subType ) );
	}
}

