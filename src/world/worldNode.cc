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

