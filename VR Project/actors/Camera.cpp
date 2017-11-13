#include "Camera.h"

Camera::Camera()
{
	theta = 0.0f;
	phi = 0.0f;
}

Camera::~Camera()
{
}

void Camera::setCamera(glm::vec3 eyePositionParam, glm::vec3 lookVectorParam, glm::vec3 upVectorParam, float fovYParam, float width, float height, float nearParam, float farParam)
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

	lookVector = glm::normalize(lookVector - position);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);
}

void Camera::UpdateAspectRatio(float aspectRatio)
{
	projMat = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
	projMat[1][1] *= -1;

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);
}

void Camera::UpdateOrbit(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdateOrbit(deltaX, deltaY, deltaZ);

	viewMat = glm::inverse(modelMat);
	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);
}

void Camera::UpdatePosition(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdatePosition(deltaX, deltaY, deltaZ);
	viewMat = glm::inverse(modelMat);

	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);
}