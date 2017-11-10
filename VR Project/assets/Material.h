#pragma once

#include "Asset.h"
#include "Texture.h"
#include "../core/Vertex.h"
#include "../actors/Light.h"

struct UniformBufferObject
{
	glm::mat4 modelMat;
	glm::mat4 viewMat;
	glm::mat4 projMat;

	glm::mat4 viewProjMat;
	glm::mat4 InvViewProjMat;
	glm::mat4 modelViewProjMat;

	glm::mat4 InvTransposeMat;

	glm::vec3 cameraWorldPos;

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

class Material : public Asset
{
public:

	Material();
	~Material();

	void addTexture(Texture* texture);

	void setShaderPaths(std::string v, std::string f, std::string c);

	void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string path);

	virtual void createDescriptorSetLayout() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSet() = 0;

	virtual void createUniformBuffer();

	virtual void creatDirectionalLightBuffer()
	{
		createBuffer(sizeof(DirectionalLight) * (int)(*pDirectionLights).size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, directionalLightBuffer, directionalLightBufferMemory);

	}

	


	VkShaderModule createShaderModule(const std::vector<char>& code);
	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent);

	virtual void cleanUp();

	virtual void cleanPipeline();

	void connectRenderPass(VkRenderPass rPass)
	{
		renderPass = rPass;
	}

	void setDirectionalLights(std::vector<DirectionalLight>*  DirectionLights)
	{
		pDirectionLights = DirectionLights;
	}
	

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkBuffer directionalLightBuffer;
	VkDeviceMemory directionalLightBufferMemory;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	std::vector<Texture*> textures;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	std::string vertexShaderPath;
	std::string fragmentShaderPath;
	std::string computeShaderPath;

	VkRenderPass renderPass;

	std::vector<DirectionalLight>* pDirectionLights;
private:
	
};

class ObjectDrawMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent);
private:

};

class LightingMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent);

	void setGbuffers(std::vector<VkImageView>*  pGBufferImageViews, VkImageView pDepthImageView)
	{
		gBufferImageViews = pGBufferImageViews;
		DepthImageView = pDepthImageView;
	}

	std::vector<VkImageView>*  gBufferImageViews;
	VkImageView DepthImageView;
private:

};

class DebugDisplayMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	void setGbuffers(std::vector<VkImageView>*  pGBufferImageViews, VkImageView pDepthImageView)
	{
		gBufferImageViews = pGBufferImageViews;
		DepthImageView = pDepthImageView;
	}

	std::vector<VkImageView>*  gBufferImageViews;
	VkImageView DepthImageView;

private:

};


class FinalRenderingMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	void setImageViews(VkImageView pImageView, VkImageView pDepthImageView)
	{
		ImageView = pImageView;
		DepthImageView = pDepthImageView;
	}

	VkImageView ImageView;
	VkImageView DepthImageView;

private:

};

class HDRHighlightMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	void setImageViews(VkImageView pImageViews, VkImageView pDepthImageView)
	{
		ImageViews = pImageViews;
		DepthImageView = pDepthImageView;
	}

	VkImageView ImageViews;
	VkImageView DepthImageView;

private:

};