#pragma once
#include <vector>
#include <memory>


class Serializable
{
	virtual std::unique_ptr<std::vector<unsigned char>> Serialize()         = 0;
	virtual bool Unserialize( std::unique_ptr<std::vector<unsigned char>> ) = 0;
};

