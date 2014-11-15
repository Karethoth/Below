#include "worldNode.hh"
#include "../logger.hh"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <sstream>
#include <atomic>
#include <iostream>

using namespace std;


// Bundling few flags to clean a bit the creation of stringstreams:
#define SS_RW_BIN std::stringstream::in | std::stringstream::out | std::stringstream::binary


WorldNode::WorldNode()
{
	// Static counter for the id, matters only on server
	static std::atomic<unsigned int> nodeIdCounter;

	id       = ++nodeIdCounter;
	type     = WORLD_NODE_OBJECT_TYPE;
	parent   = 0;
	position = glm::vec3( 0 );
	rotation = glm::quat();
	scale    = glm::vec3( 1.f );
}



WorldNode::~WorldNode()
{
	children.clear();
}



vector<string> WorldNode::GetDefaultFields()
{
	return vector<string>{ { "id", "position", "rotation", "scale" } };
}



string WorldNode::Serialize( vector<string> vars )
{
	// If we didn't receive any vars, serialize everything:
	if( vars.size() <= 0 )
	{
		vars = GetDefaultFields();
	}

	// Stream for the serialized data
	stringstream dataStream( SS_RW_BIN );


	// Total count of fields / member variables
	unsigned char fieldCount = 0;


	// For every var
	for( auto e = vars.begin(); e != vars.end(); e++ )
	{
		if( SerializeField( *e, dataStream ) )
		{
			fieldCount++;
		}
	}

	// Create the header; it's just the count of fields
	stringstream headerStream( SS_RW_BIN );
	SerializeUint8( headerStream, fieldCount );

	// Join header and the fields and return the result
	string serialized  = headerStream.str();
	serialized        += dataStream.str();

	return serialized;
}



bool WorldNode::Unserialize( string data )
{
	stringstream dataStream( SS_RW_BIN );
	dataStream << data;

	// Field count:
	uint8_t fieldCount = UnserializeUint8( dataStream );

	for( int i=0; i < fieldCount; i++ )
	{
		// Read the byte count for this field
		uint8_t fieldLength = UnserializeUint8( dataStream );

		// Fetch the field name
		string fieldName;
		string fieldData;

		if( !getline( dataStream, fieldName, ':' ) )
		{
			LOG_ERROR( "Error: WorldNode::Unserialize() failed to find field name!" );
			break;
		}


		stringstream fieldDataStream( SS_RW_BIN );

		// Calculate the amount of data and allocate the buffer:
		uint8_t fieldDataLength = fieldLength - fieldName.length() - 1;

		// If we have a value to be unserialized, push it to the stream
		if( fieldDataLength > 0 )
		{
			char *buffer = new char[fieldDataLength];

			dataStream.read( buffer, fieldDataLength );
			fieldDataStream.write( buffer, fieldDataLength );

			delete[] buffer;
		}

		// Unserialize and handle the field:
		if( !UnserializeField( fieldName, fieldDataStream ) )
		{
			return false;
		}
	}

	return true;
}



bool WorldNode::SerializeField( std::string &fieldName, std::stringstream &stream )
{
	stringstream  fieldStream( SS_RW_BIN );
	unsigned char fieldDataLength = 0;
	string        fieldDataString;


	// ID
	if( !fieldName.compare( "id" ) )
	{
		fieldStream << "id:";
		SerializeUint32( fieldStream, id );
	}


	// POSITION
	else if( !fieldName.compare( "position" ) )
	{
		auto pos = position.Get();
		fieldStream << "position:";
		SerializeFloat( fieldStream, pos.x );
		SerializeFloat( fieldStream, pos.y );
		SerializeFloat( fieldStream, pos.z );
	}


	// ROTATION
	else if( !fieldName.compare( "rotation" ) )
	{
		auto rot = rotation.Get();
		fieldStream << "rotation:";
		SerializeFloat( fieldStream, rot.x );
		SerializeFloat( fieldStream, rot.y );
		SerializeFloat( fieldStream, rot.z );
		SerializeFloat( fieldStream, rot.w );
	}


	// SCALE
	else if( !fieldName.compare( "scale" ) )
	{
		fieldStream << "scale:";
		SerializeFloat( fieldStream, scale.x );
		SerializeFloat( fieldStream, scale.y );
		SerializeFloat( fieldStream, scale.z );
	}


	// No fitting field found?
	else
	{
		LOG_ERROR( ToString(
			"SerializeField failed because '" << fieldName <<
			"' is not a field that it knows how to handle!"
		) );

		return false;
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



bool WorldNode::UnserializeField( std::string &fieldName, std::stringstream &stream )
{
	// Calculate the length
	stream.seekp( 0, ios::end );
	size_t streamLength = static_cast<size_t>( stream.tellp() );
	stream.seekp( 0, ios::beg );

	// ID
	if( !fieldName.compare( "id" ) )
	{
		if( streamLength != sizeof( unsigned int )  )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( unsigned int ) <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		id = UnserializeUint32( stream );
	}


	// POSITION
	else if( !fieldName.compare( "position" ) )
	{
		if( streamLength != sizeof( position.Get().x ) * 3 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( position.Get().x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		decltype(position.Get()) pos;
		pos.x = UnserializeFloat( stream );
		pos.y = UnserializeFloat( stream );
		pos.z = UnserializeFloat( stream );
		position.Update( pos );
	}


	// ROTATION
	else if( !fieldName.compare( "rotation" ) )
	{
		if( streamLength != sizeof( rotation.Get().x ) * 4 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( rotation.Get().x ) * 4 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		decltype(rotation.Get()) rot{};
		rot.x = UnserializeFloat( stream );
		rot.y = UnserializeFloat( stream );
		rot.z = UnserializeFloat( stream );
		rot.w = UnserializeFloat( stream );
		rotation.Update( rot );
	}


	// SCALE
	else if( !fieldName.compare( "scale" ) )
	{
		if( streamLength != sizeof( scale.x ) * 3 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( scale.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		scale.x = UnserializeFloat( stream );
		scale.y = UnserializeFloat( stream );
		scale.z = UnserializeFloat( stream );
	}


	// No such field was found
	else
	{
		LOG_ERROR( ToString(
			"UnserializeField failed because field '" << fieldName << "' is not known!"
		) );

		return false;
	}

	return true;
}



void WorldNode::UpdateModelMatrix( WorldNode *parentPtr )
{
	modelMatrix = glm::translate( position.Get() ) *
	              glm::toMat4( rotation.Get() ) *
	              glm::scale( scale );

	// If we have been given a parent, base calculations on it
	if( parentPtr )
	{
		auto pModel = parentPtr->modelMatrix;
		modelMatrix =  pModel * modelMatrix;
	}

	for( auto &child : children )
	{
		child->UpdateModelMatrix( this );
	}
}

