#pragma once

#include "worldNode.hh"


class Entity : public WorldNode,
               public Serializable
{
	unsigned int meshId;
	unsigned int textureId;


 public:
	void Render();

	// For serialization and unserialization:
	virtual SerializedData Serialize( SerializeVarList vars );
	virtual bool Unserialize( SerializedData data );
};

