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

	virtual std::vector<std::string> GetDefaultFields() override;

	virtual bool SerializeField( std::string&, std::stringstream& ) override;
	virtual bool UnserializeField( std::string&, std::stringstream& ) override;
};



// Collision checking
bool Collides( const PhysicsObject&, const PhysicsObject& );

