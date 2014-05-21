#include "worldNode.hh"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>


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



SerializedData WorldNode::Serialize( SerializeVarList vars )
{
	return nullptr;
}



bool WorldNode::Unserialize( SerializedData data )
{
	return false;
}



void WorldNode::UpdateTransformations()
{
	// Generate the local matrix
	transformMatrix = glm::translate( position ) *
		              glm::toMat4( rotation ) *
		              glm::scale( scale );

	// If we have parent, find it and get the transformation matrix,
	if( parent )
	{
		// TODO: get the matrix
		glm::mat4 parentMatrix; // Just an identity matrix for now.

		// Apply it:
		transformMatrix = transformMatrix * parentMatrix;
	}


	// Update the children
	std::for_each( children.begin(),
	               children.end(), [this]( unsigned int &childId )
	{
		// Find the child
		Serialize( nullptr );
	} );
}

