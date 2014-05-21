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



void WorldNode::UpdateModelMatrix( WorldNode *parent=nullptr )
{
	// Generate the local matrix
	modelMatrix = glm::translate( position ) *
	              glm::toMat4( rotation ) *
	              glm::scale( scale );

	// If we have parent, take it into account
	if( parent )
	{
		modelMatrix = modelMatrix * parent->GetModelMatrix();
	}


	// Update the children
	std::for_each( children.begin(),
	               children.end(),
	               [this]( unsigned int &childId )
	{
		// Find the child
		// Call update
		// child->UpdateModelMatrix( this );
	} );
}

