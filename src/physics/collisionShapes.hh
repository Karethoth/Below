#pragma once

#include "../world/worldNode.hh"
#include "../world/worldObjectTypes.hh"

enum CollisionShapeType : unsigned char
{
	COLLISION_SHAPE_NONE,
	COLLISION_SHAPE_SPHERE,
	COLLISION_SHAPE_AABB
};


// Collision Shape
struct CollisionShape
{
	CollisionShapeType type;

	union
	{
		struct
		{
			float size;
		} sphere;

		struct
		{
			float x;
			float y;
			float w;
			float h;
		} aabb;
	};
};

