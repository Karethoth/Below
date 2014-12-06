#include "clientObjectManager.hh"
#include "../logger.hh"
#include "../world/objectEvents.hh"
#include "../world/entity.hh"
#include "../physics/physicsObject.hh"

using namespace std;


void ClientObjectManager::HandleEvent( Event *e )
{
	lock_guard<std::mutex> lock( managerMutex );

	ObjectCreateEvent  *createEvent;
	ObjectUpdateEvent  *updateEvent;
	//ObjectDestroyEvent *destroyEvent;

	ObjectParentAddEvent     *parentAddEvent;
	ObjectParentRemoveEvent  *parentRemoveEvent;
	ObjectChildAddEvent      *childAddEvent;
	ObjectChildRemoveEvent   *childRemoveEvent;

	if( e->type != OBJECT_EVENT )
	{
		LOG_ERROR( "Wrong type of event was passed to ServerObjectManager::HandleEvent." );
		return;
	}

	shared_ptr<WorldNode>     newNode;
	shared_ptr<Entity>        newEntity;
	shared_ptr<PhysicsObject> newPhysicsObject;

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
				newNode = newEntity;
				entities.push_back( newEntity );
			}
			else if( createEvent->objectType == PHYSICS_OBJECT_TYPE )
			{
				newPhysicsObject = make_shared<PhysicsObject>();
				newPhysicsObject->Unserialize( createEvent->data );
				newNode = newPhysicsObject;
				entities.push_back( newPhysicsObject );
			}
			else
			{
				LOG_ERROR( "OBJECT_CREATE: Unhandled object type(" << createEvent->objectType << ")!" );
				break;
			}

			LOG( "Object created in the clientobjectmanager!" );
			worldNodes.push_back( newNode );
			break;


		case OBJECT_DESTROY:
			//destroyEvent = static_cast<ObjectDestroyEvent*>( e );
			LOG( "Object destroyed!" );
			break;


		case OBJECT_UPDATE:
			updateEvent = static_cast<ObjectUpdateEvent*>( e );

			for( auto& node : worldNodes )
			{
				if( node->id == updateEvent->objectId )
				{
					node->Unserialize( updateEvent->data );
				}
			}

			if( newNode )
			{
				newNode->Unserialize( updateEvent->data );
			}
			break;


		case OBJECT_PARENT_ADD:
			parentAddEvent = static_cast<ObjectParentAddEvent*>( e );
			AddParent( parentAddEvent->objectId, parentAddEvent->parentId );
			AddChild( parentAddEvent->parentId, parentAddEvent->objectId );
			LOG( "Parent added!" );
			break;


		case OBJECT_PARENT_REMOVE:
			parentRemoveEvent = static_cast<ObjectParentRemoveEvent*>( e );
			RemoveParent( parentRemoveEvent->objectId, parentRemoveEvent->parentId );
			RemoveChild( parentRemoveEvent->parentId, parentRemoveEvent->objectId );
			LOG( "Parent removed!" );
			break;


		case OBJECT_CHILD_ADD:
			childAddEvent = static_cast<ObjectChildAddEvent*>( e );
			AddParent( childAddEvent->objectId, childAddEvent->childId );
			AddChild( childAddEvent->childId, childAddEvent->objectId );
			LOG( "Child added!" );
			break;


		case OBJECT_CHILD_REMOVE:
			childRemoveEvent = static_cast<ObjectChildRemoveEvent*>( e );
			RemoveParent( childRemoveEvent->objectId, childRemoveEvent->childId );
			RemoveChild( childRemoveEvent->childId, childRemoveEvent->objectId );
			LOG( "Child removed!" );
			break;


		default:
			LOG( "Undefined event sub type: '" << static_cast<EVENT_SUB_TYPE>( e->subType ) );
	}
}



void ClientObjectManager::AddParent( unsigned int childId, unsigned int parentId )
{
	for( auto& node : worldNodes )
	{
		if( node->id == childId )
		{
			node->parent = parentId;
			break;
		}
	}
}



void ClientObjectManager::RemoveParent( unsigned int childId, unsigned int parentId )
{
	for( auto& node : worldNodes )
	{
		if( node->id == childId )
		{
			node->parent = 0;
			break;
		}
	}
}



void ClientObjectManager::AddChild( unsigned int parentId, unsigned int childId )
{
	shared_ptr<WorldNode> parent = nullptr;
	shared_ptr<WorldNode> child  = nullptr;

	for( auto& node : worldNodes )
	{
		if( node->id == parentId )
		{
			parent = node;
		}

		else if( node->id == childId )
		{
			child = node;
		}

		if( parent.get() && child.get() )
		{
			break;
		}
	}

	if( parent.get() && child.get() )
	{
		parent->children.push_back( child );
	}
}



void ClientObjectManager::RemoveChild( unsigned int parentId, unsigned int childId )
{
	for( auto& node : worldNodes )
	{
		if( node->id != parentId )
		{
			continue;
		}


		for( auto it=node->children.begin(); it != node->children.end(); it++ )
		{
			if( (*it)->id != childId )
			{
				continue;
			}

			node->children.erase( it );
			return;
		}
	}
}

