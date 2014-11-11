#pragma once

#include <vector>
#include <memory>
#include <sstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../network/serializable.hh"


struct WorldNode: public Serializable
{
 public:
	 WorldNode();
	 virtual ~WorldNode();

	// For serialization and unserialization:
	virtual std::string Serialize( std::vector<std::string> vars );
	virtual bool Unserialize( std::string data );

	virtual bool SerializeField( std::string &fieldName, std::stringstream &stream );
	virtual bool UnserializeField( std::string &fieldName, std::stringstream &stream  );

	// For node and entity identification:
	unsigned int id;

	// Update the model matrix:
	// - Updates childrens recursively.
	virtual void UpdateModelMatrix( WorldNode *parent=nullptr );

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 modelMatrix;

	unsigned int parent;
	std::vector<std::shared_ptr<WorldNode>> children;
};

