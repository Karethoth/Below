#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct VertexData
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct Mesh
{
	unsigned int id;

	GLuint vbo;
	GLuint vao;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;

	std::vector<VertexData> vertexData;

	void GenerateBuffers();
	void FreeVbo();
};

