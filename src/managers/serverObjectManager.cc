#include "serverObjectManager.hh"
#include "../logger.hh"
#include "../world/objectEvents.hh"

using namespace std;


void ServerObjectManager::HandleEvent( Event *e )
{
	lock_guard<std::mutex> lock( managerMutex );

	ObjectCreateEvent  *createEvent;
	ObjectUpdateEvent  *updateEvent;
	//ObjectDestroyEvent *destroyEvent;

	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ServerObjectManager::HandleEvent." );
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
			//destroyEvent = static_cast<ObjectDestroyEvent*>( e );
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

