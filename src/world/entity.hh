#pragma once

#include "worldNode.hh"


struct Entity : public WorldNode
{
	unsigned int meshId;
	unsigned int textureId;

	// For serialization and unserialization:
	virtual std::string Serialize( std::vector<std::string> vars );
	virtual bool Unserialize( std::string data );
};

