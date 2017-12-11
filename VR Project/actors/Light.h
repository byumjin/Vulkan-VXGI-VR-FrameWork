#pragma once

#include "../core/Common.h"

struct Light
{
	glm::vec4 focusPosition;
	glm::vec4 lightPosition;
	glm::vec4 lightColor; // a is for intensity
};

struct PointLight
{
	Light lightInfo;
	float lightRadius;
};


struct DirectionalLight
{
	Light lightInfo;
	glm::vec4 lightDirection;

	glm::mat4 viewMat;
	glm::mat4 projMat;

	static VkDescriptorSetLayoutBinding getDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		return uboLayoutBinding;
	}
};

static void SetDirectionLightMatrices(DirectionalLight &dlight, float width, float height, float nearParam, float farParam)
{

	dlight.lightDirection = glm::vec4(glm::normalize(glm::vec3(dlight.lightInfo.focusPosition) - glm::vec3(dlight.lightInfo.lightPosition)), 0.0);

	glm::vec3 upVector = glm::vec3(0.0, 1.0, 0.0);

	dlight.viewMat = glm::lookAt(glm::vec3(dlight.lightInfo.lightPosition), glm::vec3(dlight.lightInfo.focusPosition), upVector);
	dlight.projMat = glm::orthoRH(-width*0.5f, width*0.5f, -height*0.5f, height*0.5f, nearParam, farParam);
	dlight.projMat[1][1] *= -1;
}

static void SwingXAxisDirectionalLight(Light &lightInfo, float distance, float time, float angleCap)
{
	float xGap = glm::sin(time)*angleCap;	
	lightInfo.lightPosition.z = lightInfo.focusPosition.z + distance * xGap;

	float yGap = glm::sqrt(1.0f - xGap*xGap);
	lightInfo.lightPosition.y = lightInfo.focusPosition.y + distance * yGap;

	
}

