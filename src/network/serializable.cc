#include "serializable.hh"

using namespace std;


// Serializing helpers
void SerializeUint8( std::stringstream &stream,  uint8_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 1 );
}


void SerializeUint16( std::stringstream &stream, uint16_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 2 );
}


void SerializeUint32( std::stringstream &stream, uint32_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 4 );
}


void SerializeFloat( std::stringstream &stream, float value )
{
	stream.write( reinterpret_cast<char*>( &value ), sizeof( float ) );
}


void SerializeDouble( std::stringstream &stream, double value )
{
	stream.write( reinterpret_cast<char*>( &value ), sizeof( double ) );
}




// Unserializing helpers
uint8_t UnserializeUint8( std::stringstream &stream )
{
	uint8_t value;
	stream.read( reinterpret_cast<char*>( &value ), 1 );
	return value;
}


uint16_t UnserializeUint16( std::stringstream &stream )
{
	uint16_t value;
	stream.read( reinterpret_cast<char*>( &value ), 2 );
	return value;
}


uint32_t UnserializeUint32( std::stringstream &stream )
{
	uint32_t value;
	stream.read( reinterpret_cast<char*>( &value ), 4 );
	return value;
}


float UnserializeFloat( std::stringstream &stream )
{
	float value;
	stream.read( reinterpret_cast<char*>( &value ), sizeof( float ) );
	return value;
}


double UnserializeDouble( std::stringstream &stream )
{
	double value;
	stream.read( reinterpret_cast<char*>( &value ), sizeof( double ) );
	return value;
}

