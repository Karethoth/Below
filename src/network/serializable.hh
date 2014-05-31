#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sstream>


class Serializable
{
 public:
	virtual std::string Serialize( std::vector<std::string> vars ) = 0;
	virtual bool Unserialize( std::string data ) = 0;
};



// Few helpers:
void SerializeUint8( std::stringstream &stream,  uint8_t value );
void SerializeUint16( std::stringstream &stream, uint16_t value );
void SerializeUint32( std::stringstream &stream, uint32_t value );
void SerializeFloat( std::stringstream &stream, float value );
void SerializeDouble( std::stringstream &stream, double value );


uint8_t  UnserializeUint8( std::stringstream &stream );
uint16_t UnserializeUint16( std::stringstream &stream );
uint32_t UnserializeUint32( std::stringstream &stream );
float    UnserializeFloat( std::stringstream &stream );
double   UnserializeDouble( std::stringstream &stream );

