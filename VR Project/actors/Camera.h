#pragma once

#include "../core/Common.h"
#include "Actor.h"

class Camera : public Actor
{
public:
	Camera();
	~Camera();

	void setCamera(glm::vec3 eyePositionParam, glm::vec3 lookVectorParam, glm::vec3 upVectorParam, float fovYParam, float width, float height, float nearParam, float farParam)
	{
		position = eyePositionParam;
		lookVector = lookVectorParam;
		upVector = upVectorParam;
		fovY = fovYParam;

		nearPlane = nearParam;
		farPlane = farParam;

		viewMat = glm::lookAt(position, lookVector, upVector);
		projMat = glm::perspective(glm::radians(fovY), width / height, nearPlane, farPlane);
		projMat[1][1] *= -1;

		viewProjMat = projMat * viewMat;
		InvViewProjMat = glm::inverse(viewProjMat);
	}

	void UpdateAspectRatio(float aspectRatio)
	{
		projMat = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
		projMat[1][1] *= -1;

		viewProjMat = projMat * viewMat;
		InvViewProjMat = glm::inverse(viewProjMat);
	}

	virtual void UpdateOrbit(float deltaX, float deltaY, float deltaZ)
	{
		Actor::UpdateOrbit(deltaX, deltaY, deltaZ);

		viewMat = glm::inverse(modelMat);
		lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

		viewProjMat = projMat * viewMat;
		InvViewProjMat = glm::inverse(viewProjMat);
	}

	virtual void UpdatePosition(float deltaX, float deltaY, float deltaZ)
	{
		Actor::UpdatePosition(deltaX, deltaY, deltaZ);
		viewMat = glm::inverse(modelMat);

		lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

		viewProjMat = projMat * viewMat;
		InvViewProjMat = glm::inverse(viewProjMat);
	}

	//glm::vec3 eyePosition;
	glm::vec3 lookVector;
	
	glm::mat4 viewMat;
	glm::mat4 projMat;

	glm::mat4 viewProjMat;
	glm::mat4 InvViewProjMat;
	
	float nearPlane;
	float farPlane;

	float fovY;
private:

	
};

