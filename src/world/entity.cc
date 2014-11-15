#include "entity.hh"
#include "../logger.hh"
#include <string>
#include <sstream>

using namespace std;


#define SS_RW_BIN std::stringstream::in | std::stringstream::out | std::stringstream::binary


Entity::Entity()
{
	type = ENTITY_OBJECT_TYPE;
}



vector<string> Entity::GetDefaultFields()
{
	auto fields = WorldNode::GetDefaultFields();
	fields.push_back( "material" );
	fields.push_back( "texture" );
	fields.push_back( "mesh" );
	return fields;
}



bool Entity::SerializeField( std::string &fieldName, std::stringstream &stream )
{
	stringstream  fieldStream( SS_RW_BIN );
	unsigned char fieldDataLength = 0;
	string        fieldDataString;

	// Material
	if( !fieldName.compare( "material" ) )
	{
		fieldStream << "material:";
		SerializeFloat( fieldStream, material.color.r );
		SerializeFloat( fieldStream, material.color.g );
		SerializeFloat( fieldStream, material.color.b );
		SerializeFloat( fieldStream, material.color.a );
	}

	// Texture
	else if( !fieldName.compare( "texture" ) )
	{
		fieldStream << "texture:";
		SerializeString( fieldStream, texture );
	}

	// Mesh
	else if( !fieldName.compare( "mesh" ) )
	{
		fieldStream << "mesh:";
		SerializeString( fieldStream, texture );
	}

	// No fitting field found?
	else
	{
		return WorldNode::SerializeField( fieldName, stream );
	}

	// Handle the fieldStream to get the
	// data as a string and the length of it.
	fieldDataString = fieldStream.str();
	fieldDataLength = fieldDataString.length();

	// Do we have anything to serialize?
	if( fieldDataLength <= 0 )
	{
		return false;
	}

	// Write the header / amount of data:
	SerializeUint8( stream, fieldDataLength );

	// Write the data:
	stream << fieldDataString;

	return true;
}



bool Entity::UnserializeField( std::string &fieldName, std::stringstream &stream )
{
	// Calculate the length
	stream.seekp( 0, ios::end );
	size_t streamLength = static_cast<size_t>( stream.tellp() );
	stream.seekp( 0, ios::beg );

	// Material
	if( !fieldName.compare( "material" ) )
	{
		if( streamLength != sizeof( material.color.r ) * 4 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( material.color.r ) * 4 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		material.color.r = UnserializeFloat( stream );
		material.color.g = UnserializeFloat( stream );
		material.color.b = UnserializeFloat( stream );
		material.color.a = UnserializeFloat( stream );
	}

	else if( !fieldName.compare( "texture" ) )
	{
		if( streamLength > 0 )
		{
			texture = UnserializeString( stream );
		}
		else
		{
			texture = string{ "default" };
		}
	}

	else if( !fieldName.compare( "mesh" ) )
	{
		if( streamLength > 0 )
		{
			mesh = UnserializeString( stream );
		}
		else
		{
			mesh = string{ "default" };
		}
	}

	// No such field was found
	else
	{
		return WorldNode::UnserializeField( fieldName, stream );
	}

	return true;
}
