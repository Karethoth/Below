#pragma once

#include "../world/worldNode.hh"
#include "../world/worldObjectTypes.hh"

enum CollisionShapeType : unsigned char
{
	COLLISION_SHAPE_NONE,
	COLLISION_SHAPE_SPHERE,
	COLLISION_SHAPE_AABB
};

// Base class for collision shapes
struct CollisionShape : public WorldNode
{
	CollisionShape();

	CollisionShapeType shapeType;
};



// Collision Sphere
struct CollisionSphere : public CollisionShape
{
	CollisionSphere( float _sphereSize={ 1.f } );

	float sphereSize;
};



// Collision AABB
struct CollisionAABB : public CollisionShape
{
	CollisionAABB( glm::vec4 _boundingBox={0.f, 0.f, 1.f, 1.f} );

	glm::vec4 boundingBox;
};



// Template for Collision detection
template <typename T, typename N>
bool Collides( const T&, const N& );

// Declare specialization for Sphere-Sphere collision detection
template <>
bool Collides( const CollisionSphere& a, const CollisionSphere& b );

