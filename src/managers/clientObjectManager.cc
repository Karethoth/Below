#include "clientObjectManager.hh"
#include "../logger.hh"
#include "../world/objectEvents.hh"

using namespace std;


void ClientObjectManager::HandleEvent( Event *e )
{
	lock_guard<std::mutex> lock( managerMutex );

	ObjectCreateEvent *createEvent;
	ObjectDestroyEvent *destroyEvent;
	ObjectUpdateEvent *updateEvent;

	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ClientObjectManager::HandleEvent." );
		return;
	}

	static WorldNode *newNode;

	switch( e->subType )
	{
		case OBJECT_CREATE:
			createEvent = static_cast<ObjectCreateEvent*>( e );
			LOG( "Object creation started.." );

			newNode = new WorldNode();
			newNode->Unserialize( createEvent->data );
			LOG( "Object created!" );
			break;


		case OBJECT_DESTROY:
			destroyEvent = static_cast<ObjectDestroyEvent*>( e );
			delete newNode;
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
			LOG( "Undefined event sub type: '" << e->subType );
	}
}

