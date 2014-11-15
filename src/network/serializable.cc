#include "serializable.hh"

using namespace std;


// Serializing helpers
void SerializeUint8( stringstream &stream,  uint8_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 1 );
}


void SerializeUint16( stringstream &stream, uint16_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 2 );
}


void SerializeUint32( stringstream &stream, uint32_t value )
{
	stream.write( reinterpret_cast<char*>( &value ), 4 );
}


void SerializeFloat( stringstream &stream, float value )
{
	stream.write( reinterpret_cast<char*>( &value ), sizeof( float ) );
}


void SerializeDouble( stringstream &stream, double value )
{
	stream.write( reinterpret_cast<char*>( &value ), sizeof( double ) );
}


void SerializeString( stringstream &stream, const string& value )
{
	auto length = value.size();
	if( length <= 0 || length > 255 )
	{
		return;
	}

	SerializeUint8( stream, static_cast<uint8_t>( length ) );
	stream.write( value.c_str(), length );
}



// Unserializing helpers
uint8_t UnserializeUint8( stringstream &stream )
{
	uint8_t value;
	stream.read( reinterpret_cast<char*>( &value ), 1 );
	return value;
}


uint16_t UnserializeUint16( stringstream &stream )
{
	uint16_t value;
	stream.read( reinterpret_cast<char*>( &value ), 2 );
	return value;
}


uint32_t UnserializeUint32( stringstream &stream )
{
	uint32_t value;
	stream.read( reinterpret_cast<char*>( &value ), 4 );
	return value;
}


float UnserializeFloat( stringstream &stream )
{
	float value;
	stream.read( reinterpret_cast<char*>( &value ), sizeof( float ) );
	return value;
}


double UnserializeDouble( stringstream &stream )
{
	double value;
	stream.read( reinterpret_cast<char*>( &value ), sizeof( double ) );
	return value;
}


string UnserializeString( stringstream &stream )
{
	char buffer[255];

	// Read length
	auto stringLength = UnserializeUint8( stream );

	// Check that we have enough data left
	auto currentPos = stream.tellg();
	stream.seekg( 0, ios::end );
	auto endPos = stream.tellg();
	stream.seekg( 0, ios::cur );

	// Validate length
	auto remainingLength = endPos - currentPos;
	if( remainingLength < stringLength )
	{
		return {};
	}

	stream.read( buffer, stringLength );

	return { buffer, stringLength };
}

