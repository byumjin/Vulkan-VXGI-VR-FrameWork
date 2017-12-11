#pragma once

#include <OVR_CAPI_Vk.h>
#include <Extras/OVR_Math.h>
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
	void setHmdState(const bool bRenderToHmd, ovrFovPort* fovport);
	void UpdateMatricesHmdVR(const glm::vec3& relativeLeftPos, const glm::vec3& relativeRightPos);
	void UpdateOrbitHmdVRSampleCode(const ovrPosef* eyeRenderPoset);
	void UpdateOrbitHmdVR(const glm::vec3& deltaHmdEuler, const glm::vec3& deltaHmdPos, const glm::vec3& relativeLeftPos, const glm::vec3& relativeRightPos);
	virtual void UpdatePosition(float deltaX, float deltaY, float deltaZ);

	glm::vec3 centerPosition;
	glm::vec3 lookVector;
	
	glm::mat4 viewMat;
	glm::mat4 projMat;
	bool bRenderToHmd = false;
	ovrFovPort g_EyeFov[2];

	glm::mat4 viewProjMat;
	glm::mat4 InvViewProjMat;

	std::vector<glm::mat4> viewMatforVR;
	std::vector<glm::mat4> projMatforVR;

	std::vector<glm::mat4> viewProjMatforVR;
	std::vector<glm::mat4> InvViewProjMatforVR;
	
	float nearPlane;
	float farPlane;

	float fovY;
	float omega = 0.f;//OCULUS SDK// Needed for HMD head roll

	float IPD;
	float focalDistance;

	std::vector<glm::vec3> positionforVR;

	float aspectRatio;
	bool vrMode;

private:

	
};

