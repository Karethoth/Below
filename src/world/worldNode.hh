#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../network/serializable.hh"


class WorldNode: public Serializable
{
 public:
	 WorldNode();
	 ~WorldNode();

	// For serialization and unserialization:
	virtual SerializedData Serialize( SerializeVarList vars );
	virtual bool Unserialize( SerializedData data );

	// For node and entity identification:
	unsigned int id;


 private:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::mat4 transformMatrix;

	std::vector<unsigned int> children;

};

