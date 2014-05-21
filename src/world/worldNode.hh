#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../network/serializable.hh"


class WorldNode: public Serializable
{
 public:
	 WorldNode();
	 virtual ~WorldNode();

	// For serialization and unserialization:
	virtual SerializedData Serialize( SerializeVarList vars );
	virtual bool Unserialize( SerializedData data );

	// For node and entity identification:
	unsigned int id;

	// Update transformation matrix:
	// - Updates childrens recursively.
	virtual void UpdateTransformations();


 private:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 transformMatrix;
	glm::mat4 parentTransformMatrix;

	unsigned int parent;
	std::vector<unsigned int> children;
};

