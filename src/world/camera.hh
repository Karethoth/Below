#pragma once

#include "worldNode.hh"

class Camera : public WorldNode
{
 public:
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	glm::mat4 CalculateVPMatrix()
	{
		return projectionMatrix * viewMatrix;
	}
};

