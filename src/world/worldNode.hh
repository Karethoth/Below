#pragma once

#include <vector>
#include <memory>
#include <sstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../network/serializable.hh"
#include "worldObjectTypes.hh"
#include "../smooth.hh"


struct WorldNode: public Serializable
{
	 WorldNode();
	 virtual ~WorldNode();

	 WorldObjectType type;

	// For serialization and unserialization:
	virtual std::string Serialize( std::vector<std::string> vars={} );
	virtual bool Unserialize( std::string data );

	virtual bool SerializeField( std::string &fieldName, std::stringstream &stream );
	virtual bool UnserializeField( std::string &fieldName, std::stringstream &stream  );

	// Returns vector of the default fields to serialize
	virtual std::vector<std::string> GetDefaultFields();

	// For node and entity identification:
	unsigned int id;

	// Update the model matrix:
	// - Updates childrens recursively.
	virtual void UpdateModelMatrix( WorldNode *parent=nullptr );

	Smooth<glm::vec3> position;
	Smooth<glm::quat> rotation;
	glm::vec3 scale;
	glm::mat4 modelMatrix;

	unsigned int parent;
	std::vector<std::shared_ptr<WorldNode>> children;
};

