#include "collisionShapes.hh"


CollisionShape::CollisionShape() : shapeType( COLLISION_SHAPE_NONE )
{
	type = COLLISION_SHAPE;
}



CollisionSphere::CollisionSphere( float _sphereSize ) : sphereSize( _sphereSize )
{
	shapeType = COLLISION_SHAPE_SPHERE;
	CollisionShape::CollisionShape();
}



CollisionAABB::CollisionAABB( glm::vec4 _boundingBox ) : boundingBox( _boundingBox )
{
	shapeType = COLLISION_SHAPE_AABB;
	CollisionShape::CollisionShape();
}



template <>
bool Collides( const CollisionSphere& a, const CollisionSphere& b )
{
	auto distance = abs( glm::distance( a.position.Raw(), b.position.Raw() ) );
	return distance <= a.sphereSize + b.sphereSize;
}

