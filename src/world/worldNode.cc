#include "worldNode.hh"
#include "../logger.hh"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <sstream>
#include <atomic>

using namespace std;


// Static counter for the id, matters only on server
static std::atomic<unsigned int> nodeIdCounter;


// Bundling few flags to clean a bit the creation of stringstreams:
#define SS_RW_BIN std::stringstream::in | std::stringstream::out | std::stringstream::binary


WorldNode::WorldNode()
{
	id       = ++nodeIdCounter;
	parent   = 0;
	position = glm::vec3( 0 );
	position.y = 5;
	position.z = -5;
	rotation = glm::quat();
	scale    = glm::vec3( 1.f );
}



WorldNode::~WorldNode()
{
}



string WorldNode::Serialize( vector<string> vars )
{
	// TODO: Serialize just the given fields, if we got any.
	//       Otherwise serialize every field.


	// Stream for the serialized data
	stringstream dataStream( SS_RW_BIN );


	// Total count of fields / member variables
	unsigned char fieldCount = 0;


	// Per field variables:
	string        fieldData;
	unsigned char fieldDataLength;
	stringstream  fieldStream;


	// Id:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "id:";
	SerializeUint32( fieldStream, id );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	SerializeUint8( dataStream, fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Position:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "position:";
	SerializeFloat( fieldStream, position.x );
	SerializeFloat( fieldStream, position.y );
	SerializeFloat( fieldStream, position.z );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	SerializeUint8( dataStream, fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Rotation:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "rotation:";
	SerializeFloat( fieldStream, rotation.x );
	SerializeFloat( fieldStream, rotation.y );
	SerializeFloat( fieldStream, rotation.z );
	SerializeFloat( fieldStream, rotation.w );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	SerializeUint8( dataStream, fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Scale:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "scale:";
	SerializeFloat( fieldStream, scale.x );
	SerializeFloat( fieldStream, scale.y );
	SerializeFloat( fieldStream, scale.z );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	SerializeUint8( dataStream, fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


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

		// Calculate the amount of data and allocate the buffer:
		uint8_t fieldDataLength = fieldLength - fieldName.length() - 1;
		assert( fieldDataLength > 0 );
		char *buffer = new char[fieldLength];

		// Copy the field data to other stream
		stringstream fieldDataStream( SS_RW_BIN );
		dataStream.read( buffer, fieldDataLength );
		fieldDataStream.write( buffer, fieldDataLength );

		// Free the memory for buffer
		delete[] buffer;

		// Unserialize and handle the field:
		if( !UnserializeField( fieldName, fieldDataStream ) )
		{
			return false;
		}
	}

	return false;
}



bool WorldNode::SerializeField( std::string &fieldName, std::stringstream &stream )
{
	return false;
}



bool WorldNode::UnserializeField( std::string &fieldName, std::stringstream &stream )
{
	// Calculate the length
	stream.seekp( 0, ios::end );
	size_t streamLength = stream.tellp();
	stream.seekp( 0, ios::beg );
	assert( streamLength > 0 );


	LOG( ToString( "Got field " << fieldName << "(" << (int)streamLength << ") with value '" \
		                        << stream.str() << "'" ) );

	// ID
	if( !fieldName.compare( "id" ) )
	{
		if( streamLength != (size_t)(sizeof( unsigned int ) ) )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( unsigned int ) <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		id = UnserializeUint32( stream );

		LOG( ToString( "Result: id = " << id << endl ) );
	}


	// POSITION
	else if( !fieldName.compare( "position" ) )
	{
		if( streamLength != (size_t)(sizeof( position.x ) * 3) )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( position.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		position.x = UnserializeFloat( stream );
		position.y = UnserializeFloat( stream );
		position.z = UnserializeFloat( stream );

		LOG( ToString(
			"Result: position = <" <<
			position.x << "," <<
			position.y << "," <<
			position.z << ">" <<
			endl
		) );
	}


	// ROTATION
	else if( !fieldName.compare( "rotation" ) )
	{
		if( streamLength != sizeof( rotation.x ) * 4 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( position.x ) * 4 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		rotation.x = UnserializeFloat( stream );
		rotation.y = UnserializeFloat( stream );
		rotation.z = UnserializeFloat( stream );
		rotation.w = UnserializeFloat( stream );

		LOG( ToString(
			"Result: rotation = <" <<
			rotation.x << "," <<
			rotation.y << "," <<
			rotation.z << "," <<
			rotation.w << ">" <<
			endl
		) );
	}


	// SCALE
	else if( !fieldName.compare( "scale" ) )
	{
		if( streamLength != sizeof( scale.x ) * 3 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( position.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		scale.x = UnserializeFloat( stream );
		scale.y = UnserializeFloat( stream );
		scale.z = UnserializeFloat( stream );

		LOG( ToString(
			"Result: scale = <" <<
			scale.x << "," <<
			scale.y << "," <<
			scale.z << ">" <<
			endl
		) );
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



glm::mat4 WorldNode::GetModelMatrix()
{
	return modelMatrix;
}



void WorldNode::UpdateModelMatrix( WorldNode *parentPtr=nullptr )
{
	// Generate the local matrix
	modelMatrix = glm::translate( position ) *
	              glm::toMat4( rotation ) *
	              glm::scale( scale );

	// If we have parent, take it into account
	if( parentPtr )
	{
		modelMatrix = modelMatrix * parentPtr->GetModelMatrix();
	}

	// If a pointer wasn't provided, but we have an ID for the parent
	else if( parent )
	{
		// TODO: Find the parent and apply the model matrix to it's.
	}


	// Update the children
	std::for_each( children.begin(),
	               children.end(),
	               [this]( unsigned int &childId )
	{
		// Find the child
		// child->UpdateModelMatrix( this );
	} );
}

