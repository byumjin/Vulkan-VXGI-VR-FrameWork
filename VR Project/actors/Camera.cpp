#include "Camera.h"


using namespace OVR;

Camera::Camera():IPD(0.1f), focalDistance(10.0f)
{
	viewMatforVR.resize(2);
	projMatforVR.resize(2);

	viewProjMatforVR.resize(2);
	InvViewProjMatforVR.resize(2);

	positionforVR.resize(2);
}

Camera::~Camera()
{
}

void Camera::setHmdState(const bool renderHmd, ovrFovPort* fovport) {
	bRenderToHmd = renderHmd;
	g_EyeFov[0] = fovport[0];
	g_EyeFov[1] = fovport[1];
	Matrix4f projleft	= ovrMatrix4f_Projection(g_EyeFov[ovrEye_Left], NEAR_PLANE, FAR_PLANE, ovrProjection_None);
	Matrix4f projright	= ovrMatrix4f_Projection(g_EyeFov[ovrEye_Right], NEAR_PLANE, FAR_PLANE, ovrProjection_None);

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			projMatforVR[LEFT_EYE][x][y] = projleft.M[y][x];
			projMatforVR[RIGHT_EYE][x][y] = projright.M[y][x];
		}
	}
	projMatforVR[LEFT_EYE][1][1] *= -1.f;
	projMatforVR[RIGHT_EYE][1][1] *= -1.f;
}


void Camera::setCamera(glm::vec3 eyePositionParam, glm::vec3 lookVectorParam, glm::vec3 upVectorParam, float fovYParam, float width, float height, float nearParam, float farParam)
{
	position = eyePositionParam;
	centerPosition = lookVectorParam;
	upVector = upVectorParam;
	fovY = fovYParam;

	nearPlane = nearParam;
	farPlane = farParam;

	aspectRatio = width / height;

	viewMat = glm::lookAt(position, centerPosition, upVector);
	projMat = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
	projMat[1][1] *= -1;

	lookVector = glm::normalize(centerPosition - position);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));

	
	float halfFovX = glm::radians(aspectRatio * fovY * 0.5f);
	  

	positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;
	positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;

	viewMatforVR[LEFT_EYE] = glm::lookAt(positionforVR[LEFT_EYE], centerPosition - glm::length(IPD) * rightVec*0.5f, upVector);
	viewMatforVR[RIGHT_EYE] = glm::lookAt(positionforVR[RIGHT_EYE], centerPosition + glm::length(IPD) * rightVec*0.5f, upVector);

	//viewMatforVR[LEFT_EYE] = glm::lookAt(positionforVR[LEFT_EYE], position + lookVector * focalDistance, upVector);
	//viewMatforVR[RIGHT_EYE] = glm::lookAt(positionforVR[RIGHT_EYE], position + lookVector * focalDistance, upVector);

	projMatforVR[LEFT_EYE] = glm::perspective(glm::radians(fovY), width*0.5f / height, nearPlane, farPlane);
	projMatforVR[LEFT_EYE][1][1] *= -1;
	projMatforVR[RIGHT_EYE] = projMatforVR[LEFT_EYE];

	viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
}

void Camera::UpdateAspectRatio(float aspectRatioParam)
{
	aspectRatio = aspectRatioParam;

	projMat = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
	projMat[1][1] *= -1;

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	
	projMatforVR[LEFT_EYE] = glm::perspective(glm::radians(fovY), aspectRatio*0.5f, nearPlane, farPlane);
	projMatforVR[LEFT_EYE][1][1] *= -1;

	viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
}

void Camera::UpdateOrbit(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdateOrbit(deltaX, deltaY, deltaZ);
	viewMat = glm::inverse(modelMat);
	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);


	glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));


	float halfFovX = glm::radians(aspectRatio * fovY * 0.5f);


	positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;
	positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;

	glm::mat4 tempMat = modelMat;
	tempMat[3] = glm::vec4(positionforVR[LEFT_EYE], 1.0f);
	viewMatforVR[LEFT_EYE] = glm::inverse(tempMat);

	tempMat = modelMat;
	tempMat[3] = glm::vec4(positionforVR[RIGHT_EYE], 1.0f);
	viewMatforVR[RIGHT_EYE] = glm::inverse(tempMat);

	///????
	//positionforVR[RIGHT_EYE] = position - glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;
	//positionforVR[LEFT_EYE] = position + glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;

	viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
	
}

void Camera::UpdateMatricesHmdVR(const glm::vec3& relativeLeftPos, const glm::vec3& relativeRightPos) {
	viewMat = glm::inverse(modelMat);
	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);


	glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));


	float halfFovX = glm::radians(aspectRatio * fovY * 0.5f);


	positionforVR[LEFT_EYE] = position + relativeLeftPos;
	positionforVR[RIGHT_EYE] = position + relativeRightPos;

	glm::mat4 tempMat = modelMat;
	tempMat[3] = glm::vec4(positionforVR[LEFT_EYE], 1.0f);
	viewMatforVR[LEFT_EYE] = glm::inverse(tempMat);

	tempMat = modelMat;
	tempMat[3] = glm::vec4(positionforVR[RIGHT_EYE], 1.0f);
	viewMatforVR[RIGHT_EYE] = glm::inverse(tempMat);

	viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
}

void Camera::UpdateOrbitHmdVR(const glm::vec3& deltaHmdEuler, const glm::vec3& deltaHmdPos, const glm::vec3& relativeLeftPos, const glm::vec3& relativeRightPos) {
	phi += glm::degrees(deltaHmdEuler.x);
	theta += glm::degrees(deltaHmdEuler.y);
	omega += glm::degrees(deltaHmdEuler.z);

	float radTheta = glm::radians(theta);
	float radPhi = glm::radians(phi);
	float radOmega = glm::radians(omega);

	rotation = glm::rotate(glm::mat4(1.0f), radTheta, glm::vec3(0.0f, 1.0f, 0.0f)) *
		       glm::rotate(glm::mat4(1.0f), radPhi, glm::vec3(1.0f, 0.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), radOmega, glm::vec3(0.0f, 0.0f, 1.0f));
	position += deltaHmdPos;
	modelMat = glm::translate(glm::mat4(1.0f), position) * rotation *  glm::scale(glm::mat4(1.0f), scale);
	UpdateMatricesHmdVR(relativeLeftPos, relativeRightPos);
}

void Camera::UpdateOrbitHmdVRSampleCode(const ovrPosef* eyeRenderPose) {
	//Quatf poseQuat = pose.Orientation;
	//poseQuat.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z, OVR::RotateDirection::Rotate_CCW, OVR::HandedSystem::Handed_R>(&currHmdEuler.y, &currHmdEuler.x, &currHmdEuler.z);
	// Get view and projection matrices
	float radTheta = glm::radians(theta);
	float radPhi = glm::radians(phi);
	float radOmega = glm::radians(omega);

	rotation = glm::rotate(glm::mat4(1.0f), radTheta, glm::vec3(0.0f, 1.0f, 0.0f)) *
		       glm::rotate(glm::mat4(1.0f), radPhi, glm::vec3(1.0f, 0.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), radOmega, glm::vec3(0.0f, 0.0f, 1.0f));

	Matrix4f rollPitchYaw;
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			rollPitchYaw.M[x][y] = rotation[y][x];
		}
	}
	Vector3f playerPos(position.x, position.y, position.z);
	Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[ovrEye_Left].Orientation);
	Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
	Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
	Vector3f shiftedEyePos = playerPos + rollPitchYaw.Transform(eyeRenderPose[ovrEye_Left].Position);
	Matrix4f viewLeft = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			viewMatforVR[LEFT_EYE][x][y] = viewLeft.M[y][x];
		}
	}

	finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[ovrEye_Right].Orientation);
	finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
	finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
	shiftedEyePos = playerPos + rollPitchYaw.Transform(eyeRenderPose[ovrEye_Right].Position);
	Matrix4f viewRight = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			viewMatforVR[RIGHT_EYE][x][y] = viewRight.M[y][x];
		}
	}

	//lookVector = glm::vec3(viewMatforVR[LEFT_EYE][2].x, viewMatforVR[LEFT_EYE][2].y, viewMatforVR[LEFT_EYE][2].z);
	//upVector = glm::vec3(viewMatforVR[LEFT_EYE][1].x, viewMatforVR[LEFT_EYE][1].y, viewMatforVR[LEFT_EYE][1].z);
	//glm::vec3 rightVec = glm::vec3(viewMatforVR[LEFT_EYE][0].x, viewMatforVR[LEFT_EYE][0].y, viewMatforVR[LEFT_EYE][0].z);
	////lookVector = glm::vec3(modelMat[2].x, modelMat[2].y, modelMat[2].z);
	////upVector = glm::vec3(modelMat[1].x, modelMat[1].y, modelMat[1].z);
	////glm::vec3 rightVec = glm::vec3(modelMat[0].x, modelMat[0].y, modelMat[0].z);
	//float llook = glm::length(lookVector);
	//float lup = glm::length(upVector);
	//float lright = glm::length(rightVec);
	//if (llook < 0.99f || llook > 1.01 ||
	//	lup < 0.99f || lup > 1.01 ||
	//	lright < 0.99f || lright > 1.01	) 
	//{
	//	std::cout << "\nlens not normal";
	//}

	//float dotright = glm::dot(glm::cross(upVector, lookVector), rightVec);
	//float dotup		= glm::dot(glm::cross(lookVector, rightVec), upVector);
	//float dotlook = glm::dot(glm::cross(rightVec, upVector),  lookVector);
	//if (dotlook < 0.99f || dotlook > 1.01 ||
	//	dotup < 0.99f || dotup > 1.01 ||
	//	dotright < 0.99f || dotright > 1.01	) 
	//{
	//	std::cout << "\nvecs not ortho";
	//}

	glm::mat4 modelMatLeft = glm::inverse(viewMatforVR[LEFT_EYE]);
	glm::mat4 modelMatRight = glm::inverse(viewMatforVR[LEFT_EYE]);
	positionforVR[LEFT_EYE] = glm::vec3(modelMatLeft[3]);
	positionforVR[RIGHT_EYE] = glm::vec3(modelMatRight[3]);

	viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
	viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

	InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
	InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
}

void Camera::UpdatePosition(float deltaX, float deltaY, float deltaZ)
{
	Actor::UpdatePosition(deltaX, deltaY, deltaZ);
	viewMat = glm::inverse(modelMat);

	lookVector = glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z);

	viewProjMat = projMat * viewMat;
	InvViewProjMat = glm::inverse(viewProjMat);

	
		glm::vec3 rightVec = glm::normalize(glm::cross(lookVector, upVector));

		float halfFovX = glm::radians(aspectRatio * fovY * 0.5f);

		positionforVR[LEFT_EYE] = position - glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;
		positionforVR[RIGHT_EYE] = position + glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;

		glm::mat4 tempMat = modelMat;
		tempMat[3] = glm::vec4(positionforVR[LEFT_EYE], 1.0f);
		viewMatforVR[LEFT_EYE] = glm::inverse(tempMat);

		tempMat = modelMat;
		tempMat[3] = glm::vec4(positionforVR[RIGHT_EYE], 1.0f);
		viewMatforVR[RIGHT_EYE] = glm::inverse(tempMat);

		///????
		//positionforVR[RIGHT_EYE] = position - glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;
		//positionforVR[LEFT_EYE] = position + glm::length(IPD) * rightVec*0.5f + (IPD / sin(halfFovX) * 0.5f) * lookVector;

		viewProjMatforVR[LEFT_EYE] = projMatforVR[LEFT_EYE] * viewMatforVR[LEFT_EYE];
		viewProjMatforVR[RIGHT_EYE] = projMatforVR[RIGHT_EYE] * viewMatforVR[RIGHT_EYE];

		InvViewProjMatforVR[LEFT_EYE] = glm::inverse(viewProjMatforVR[LEFT_EYE]);
		InvViewProjMatforVR[RIGHT_EYE] = glm::inverse(viewProjMatforVR[RIGHT_EYE]);
	
}