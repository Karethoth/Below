#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../network/serializable.hh"


class WorldNode: public Serializable
{
 public:
	 WorldNode();
	 virtual ~WorldNode();

	// For serialization and unserialization:
	virtual std::string Serialize( std::vector<std::string> vars );
	virtual bool Unserialize( std::string data );

	// For node and entity identification:
	unsigned int id;

	glm::mat4 GetModelMatrix();

	// Update the model matrix:
	// - Updates childrens recursively.
	virtual void UpdateModelMatrix( WorldNode *parent );


 private:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 modelMatrix;

	unsigned int parent;
	std::vector<unsigned int> children;
};

