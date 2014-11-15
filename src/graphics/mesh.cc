#include "mesh.hh"


void Mesh::GenerateBuffers()
{
	if( vertices.size() <= 0 )
	{
		return;
	}

	for( size_t i=0; i < vertices.size(); i++ )
	{
		vertexData.push_back( { vertices[i], normals[i], texCoords[i] } );
	}

	// Create vertex data buffer
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );

	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof( VertexData ),
		&vertexData[0],
		GL_STATIC_DRAW
	);

	// Crete vertex array object
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );

	glVertexAttribPointer(
		0, //shaderProgramManager->Get( "guiShader" )->GetAttribute( "vertexPosition" ), // Index
		3,        // Size
		GL_FLOAT, // Type
		GL_FALSE, // Normalized?
		sizeof( VertexData ), // Stride
		(void*)0  // Offset
	);

	glVertexAttribPointer(
		1, //shaderProgramManager->Get( "guiShader" )->GetAttribute( "vertexPosition" ), // Index
		3,        // Size
		GL_FLOAT, // Type
		GL_FALSE, // Normalized?
		sizeof( VertexData ), // Stride
		(void*)sizeof(glm::vec3)  // Offset
	);

	glVertexAttribPointer(
		2, //shaderProgramManager->Get( "guiShader" )->GetAttribute( "vertexPosition" ), // Index
		2,        // Size
		GL_FLOAT, // Type
		GL_FALSE, // Normalized?
		sizeof( VertexData ), // Stride
		(void*)(sizeof(glm::vec3)*2)  // Offset
	);

	glDisableVertexAttribArray( 2 );
	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 0 );

}



void Mesh::FreeVbo()
{
	glDeleteVertexArrays( 1, &vao );
	glDeleteBuffers( 1, &vbo );
}

