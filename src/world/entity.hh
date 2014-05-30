#pragma once

#include "worldNode.hh"


class Entity : public WorldNode
{
	unsigned int meshId;
	unsigned int textureId;


 public:
	void Render();

	// For serialization and unserialization:
	virtual std::string Serialize( std::vector<std::string> vars );
	virtual bool Unserialize( std::string data );
};

