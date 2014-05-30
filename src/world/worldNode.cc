#include "worldNode.hh"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <sstream>

using namespace std;


// Bundling few flags to clean a bit the creation of stringstreams:
#define SS_RW_BIN std::stringstream::in | std::stringstream::out | std::stringstream::binary


WorldNode::WorldNode()
{
	id       = 0;
	parent   = 0;
	position = glm::vec3( 0 );
	rotation = glm::quat();
	scale    = glm::vec3( 1.f );
}



WorldNode::~WorldNode()
{
}



string WorldNode::Serialize( vector<string> vars )
{
	// TODO: serialize just given fields, if we got any


	// Stream for the serialized data
	stringstream dataStream( SS_RW_BIN );


	// Total count of fields / member variables
	unsigned char fieldCount = 0;


	// Per field variables:
	string        fieldData;
	unsigned char fieldDataLength;
	stringstream  fieldStream;


	// Position:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "position:";
	fieldStream.write( reinterpret_cast<char*>( &position.x ), sizeof( position.x ) );
	fieldStream.write( reinterpret_cast<char*>( &position.y ), sizeof( position.y ) );
	fieldStream.write( reinterpret_cast<char*>( &position.z ), sizeof( position.z ) );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	dataStream.write( reinterpret_cast<char*>( &fieldDataLength ), sizeof fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Rotation:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "rotation:";
	fieldStream.write( reinterpret_cast<char*>( &rotation.x ), sizeof( rotation.x ) );
	fieldStream.write( reinterpret_cast<char*>( &rotation.y ), sizeof( rotation.y ) );
	fieldStream.write( reinterpret_cast<char*>( &rotation.z ), sizeof( rotation.z ) );
	fieldStream.write( reinterpret_cast<char*>( &rotation.w ), sizeof( rotation.w ) );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	dataStream.write( reinterpret_cast<char*>( &fieldDataLength ), sizeof fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Scale:
	fieldStream = stringstream( SS_RW_BIN );
	fieldStream << "scale:";
	fieldStream.write( reinterpret_cast<char*>( &scale.x ), sizeof( scale.x ) );
	fieldStream.write( reinterpret_cast<char*>( &scale.y ), sizeof( scale.y ) );
	fieldStream.write( reinterpret_cast<char*>( &scale.z ), sizeof( scale.z ) );

	fieldData       = fieldStream.str();
	fieldDataLength = fieldData.length();

	dataStream.write( reinterpret_cast<char*>( &fieldDataLength ), sizeof fieldDataLength );
	dataStream << fieldData;
	fieldCount++;


	// Create the header; it's just the count of fields
	stringstream headerStream( SS_RW_BIN );
	headerStream.write( reinterpret_cast<char*>( &fieldCount ), sizeof( fieldCount ) );

	// Join header and the fields and return the result
	string serialized  = headerStream.str();
	serialized        += dataStream.str();

	return serialized;
}



bool WorldNode::Unserialize( string data )
{
	return false;
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

