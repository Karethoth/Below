#pragma once

#include <vector>

#include "../network/serializable.hh"


class WorldNode: public Serializable
{
	std::vector<WorldNode*> children;


 public:
	// position, rotation, scale via GLM

	// For serialization and unserialization:
	virtual SerializedData Serialize( SerializeVarList vars );
	virtual bool Unserialize( SerializedData data );
};

