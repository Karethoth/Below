#include "clientObjectManager.hh"
#include "../logger.hh"
#include "../world/objectEvents.hh"


void ClientObjectManager::HandleEvent( Event *e )
{
	CreateEvent *createEvent;
	DestroyEvent *destroyEvent;
	UpdateEvent *updateEvent;

	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ClientObjectManager::HandleEvent." );
		return;
	}

	switch( e->subType )
	{
		case OBJECT_CREATE:
			createEvent = static_cast<CreateEvent*>( e );
			LOG( "Object created!" );
			break;


		case OBJECT_DESTROY:
			destroyEvent = static_cast<DestroyEvent*>( e );
			LOG( "Object destroyed!" );
			break;


		case OBJECT_UPDATE:
			updateEvent = static_cast<UpdateEvent*>( e );
			LOG( "Object updated!" );
			break;


		default:
			LOG( "Undefined event sub type: '" << e->subType );
	}
}

