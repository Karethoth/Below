#include "clientObjectManager.hh"
#include "../logger.hh"


void ClientObjectManager::HandleEvent( Event *e )
{
	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ClientObjectManager::HandleEvent." );
		return;
	}


	switch( e->subType )
	{
		case OBJECT_CREATION:
			LOG( "Object created!" );
			break;

		case OBJECT_DESTROYED:
			LOG( "Object destroyed!" );
			break;

		case OBJECT_UPDATE:
			LOG( "Object updated!" );
			break;

		default:
			LOG( "Undefined event sub type: '" << e->subType );
	}
}

