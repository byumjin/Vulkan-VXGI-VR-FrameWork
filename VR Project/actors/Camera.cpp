#include "Camera.h"

Camera::Camera():IPD(1.0f)
{
	viewMatforVR.resize(2);

	viewProjMatforVR.resize(2);
	InvViewProjMatforVR.resize(2);

	positionforVR.resize(2);
}

Camera::~Camera()
{
}

void Camera::setCamera(glm::vec3 eyePositionParam, glm::vec3 lookVectorParam, glm::vec3 upVectorParam, float fovYParam, float width, float height, float nearParam, float farParam)
{
	position = eyePositionParam;
	centerPosition = lookVectorParam;
	upVector = upVectorParam;
	fovY = fovYParam;

	nearPlane = nearParam;
	farPlane = farParam;

	viewMat = glm::lookAt(position, centerPosition, upVector);
	projMat = glm::perspective(glm::radians(fovY), width / height, nearPlane, farPlane);
	projMat[1][1] *= -1;

	lookVector = glm::normalize(centerPosition - position);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));

	positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f;
	positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f;

	viewMatforVR[LEFT_EYE] = glm::lookAt(positionforVR[LEFT_EYE], centerPosition - glm::length(IPD) * rightVec*0.5f, upVector);
	viewMatforVR[RIGHT_EYE] = glm::lookAt(positionforVR[RIGHT_EYE], centerPosition + glm::length(IPD) * rightVec*0.5f, upVector);

	projMatforVR = glm::perspective(glm::radians(fovY), width*0.5f / height, nearPlane, farPlane);
	projMatforVR[1][1] *= -1;

	viewProjMatforVR[LEFT_EYE] = projMatforVR * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
}

void Camera::UpdateAspectRatio(float aspectRatio)
{
	projMat = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
	projMat[1][1] *= -1;

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	//if (vr_mode)
	//{
		projMatforVR = glm::perspective(glm::radians(fovY), aspectRatio*0.5f, nearPlane, farPlane);
		projMatforVR[1][1] *= -1;

		viewProjMatforVR[LEFT_EYE] = projMatforVR * viewMatforVR[LEFT_EYE];
		viewProjMatforVR[RIGHT_EYE] = projMatforVR * viewMatforVR[RIGHT_EYE];

		InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
		InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
	//}
	
}

void Camera::UpdateOrbit(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdateOrbit(deltaX, deltaY, deltaZ);

	viewMat = glm::inverse(modelMat);
	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	//if (vr_mode)
	//{
		glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));

		positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f;
		positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f;

		glm::mat4 tempMat = viewMat;
		tempMat[3] = glm::vec4(positionforVR[RIGHT_EYE], 1.0f);
		viewMatforVR[LEFT_EYE] = glm::inverse(tempMat);// glm::lookAt(positionforVR[LEFT_EYE], lookVector, upVector);
		tempMat[3] = glm::vec4(positionforVR[LEFT_EYE], 1.0f);
		viewMatforVR[RIGHT_EYE] = glm::inverse(tempMat);// glm::lookAt(positionforVR[RIGHT_EYE], lookVector, upVector);
		

		viewProjMatforVR[LEFT_EYE] = projMatforVR * viewMatforVR[LEFT_EYE];
		viewProjMatforVR[RIGHT_EYE] = projMatforVR * viewMatforVR[RIGHT_EYE];

		InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
		InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
	//}
}

void Camera::UpdatePosition(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdatePosition(deltaX, deltaY, deltaZ);
	viewMat = glm::inverse(modelMat);

	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	//if (vr_mode)
	//{
		glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));

		positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f;
		positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f;

		glm::mat4 tempMat = viewMat;
		tempMat[3] = glm::vec4(positionforVR[RIGHT_EYE], 1.0f);
		viewMatforVR[LEFT_EYE] = glm::inverse(tempMat);// glm::lookAt(positionforVR[LEFT_EYE], lookVector, upVector);
		tempMat[3] = glm::vec4(positionforVR[LEFT_EYE], 1.0f);
		viewMatforVR[RIGHT_EYE] = glm::inverse(tempMat);// glm::lookAt(positionforVR[RIGHT_EYE], lookVector, upVector);

		viewProjMatforVR[LEFT_EYE] = projMatforVR * viewMatforVR[LEFT_EYE];
		viewProjMatforVR[RIGHT_EYE] = projMatforVR * viewMatforVR[RIGHT_EYE];

		InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
		InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
	//}
}