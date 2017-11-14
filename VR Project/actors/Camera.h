#pragma once

#include "../core/Common.h"
#include "Actor.h"

class Camera : public Actor
{
public:
	Camera();
	~Camera();

	void setCamera(glm::vec3 eyePositionParam, glm::vec3 lookVectorParam, glm::vec3 upVectorParam, float fovYParam, float width, float height, float nearParam, float farParam);
	
	void setIPD(float param)
	{
		IPD = param;
	}

	void UpdateAspectRatio(float aspectRatio);
	
	virtual void UpdateOrbit(float deltaX, float deltaY, float deltaZ);

	virtual void UpdatePosition(float deltaX, float deltaY, float deltaZ);

	glm::vec3 centerPosition;
	glm::vec3 lookVector;
	
	glm::mat4 viewMat;
	glm::mat4 projMat;

	glm::mat4 viewProjMat;
	glm::mat4 InvViewProjMat;

	std::vector<glm::mat4> viewMatforVR;
	glm::mat4 projMatforVR;

	std::vector<glm::mat4> viewProjMatforVR;
	std::vector<glm::mat4> InvViewProjMatforVR;
	
	float nearPlane;
	float farPlane;

	float fovY;

	float IPD;

	std::vector<glm::vec3> positionforVR;


	bool vr_mode;

private:

	
};

