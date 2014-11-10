#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <iostream>


class OBJ
{
 public:
	OBJ();

	bool Load( const std::string &filepath );

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
};

