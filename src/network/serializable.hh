#pragma once
#include <vector>
#include <memory>

typedef std::unique_ptr<std::vector<std::string>> SerializeVarList;
typedef std::unique_ptr<std::vector<unsigned char>> SerializedData;


class Serializable
{
 public:
	virtual SerializedData Serialize( SerializeVarList vars ) = 0;
	virtual bool Unserialize( SerializedData data ) = 0;
};

