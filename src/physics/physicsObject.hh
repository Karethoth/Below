#pragma once

#define GLM_FORCE_RADIANS

#include "collisionShapes.hh"
#include "../world/worldNode.hh"
#include "../world/worldObjectTypes.hh"
#include "../world/entity.hh"

#include <glm/glm.hpp>
#include <memory>


struct PhysicsObject : public Entity
{
	glm::vec3 velocity;
	glm::quat angularVelocity;
	float mass;
	CollisionShape collisionShape;


	PhysicsObject();

	virtual std::vector<std::string> PhysicsObject::GetDefaultFields() override;

	virtual bool PhysicsObject::SerializeField( std::string&, std::stringstream& ) override;
	virtual bool PhysicsObject::UnserializeField( std::string&, std::stringstream& ) override;
};



// Collision checking
bool Collides( const PhysicsObject&, const PhysicsObject& );

