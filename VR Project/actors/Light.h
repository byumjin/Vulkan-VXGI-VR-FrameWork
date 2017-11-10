#pragma once

#include "../core/Common.h"

struct Light
{
	glm::vec4 lightPosition;
	glm::vec4 lightColor; // a is for intensity
};

struct DirectionalLight
{
	Light lightInfo;
	glm::vec3 lightDirection;

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

struct PointLight
{
	Light lightInfo;
	float lightRadius;
};
