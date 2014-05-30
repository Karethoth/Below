#pragma once
#include <string>
#include <vector>
#include <memory>


class Serializable
{
 public:
	virtual std::string Serialize( std::vector<std::string> vars ) = 0;
	virtual bool Unserialize( std::string data ) = 0;
};

