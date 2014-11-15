#pragma once

#include "worldNode.hh"

struct Material
{
	glm::vec4 color;
};


struct Entity : public WorldNode
{
	Entity();

	unsigned int meshId;
	unsigned int textureId;
	Material     material;

	// For serialization and unserialization:
	virtual bool SerializeField( std::string &fieldName, std::stringstream &stream ) override;
	virtual bool UnserializeField( std::string &fieldName, std::stringstream &stream  ) override;

	virtual std::vector<std::string> GetDefaultFields() override;
};

