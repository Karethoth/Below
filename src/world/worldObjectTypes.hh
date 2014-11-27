#pragma once

#define WORLD_OBJECT_TYPE_LENGTH sizeof( unsigned char );


enum WorldObjectType : unsigned char
{
	UNDEF_OBJECT_TYPE = 0,
	WORLD_NODE_OBJECT_TYPE,
	ENTITY_OBJECT_TYPE,
	COLLISION_SHAPE,
};

