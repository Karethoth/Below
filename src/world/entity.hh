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
	//virtual std::string Serialize( std::vector<std::string> vars );
	//virtual bool Unserialize( std::string data );
};

