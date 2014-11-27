#pragma once

#include "collisionShapes.hh"
#include "../world/worldNode.hh"
#include "../world/worldObjectTypes.hh"
#include "../world/entity.hh"

#include <glm/glm.hpp>
#include <memory>


struct PhysicsObject : public Entity
{
	glm::vec3 velocity;
	glm::vec3 angularVelocity;

	float mass;

	std::unique_ptr<CollisionShape> collisionShape;

	virtual bool SerializeField( std::string &fieldName, std::stringstream &stream ) override;
	virtual bool UnserializeField( std::string &fieldName, std::stringstream &stream  ) override;
};

