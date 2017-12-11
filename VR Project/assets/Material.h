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

struct BlurUniformBufferObject
{
	float widthGap;
	float heightGap;
};

struct ShadowUniformBuffer
{
	glm::mat4 viewProjMat;
	glm::mat4 invViewProjMat;
};

struct ObjectUniformBuffer
{
	glm::mat4 modelMat;
};




class Material : public Asset
{
public:

	Material();
	virtual ~Material();

	void addTexture(Texture* texture);

	void setShaderPaths(std::string v, std::string f, std::string c, std::string e, std::string g, std::string p);

	void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam);

	virtual void createDescriptorSetLayout() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSet() = 0;

	virtual void createUniformBuffer();

	virtual void creatDirectionalLightBuffer()
	{
		createBuffer(sizeof(DirectionalLight) * (int)(*pDirectionLights).size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, directionalLightBuffer, directionalLightBufferMemory);

	}

	VkShaderModule createShaderModule(const std::vector<char>& code);
	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent){}
	virtual void createComputePipeline(VkExtent2D swapChainExtent){}

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
	
	void setScreenScale(glm::vec2 scale);

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkBuffer directionalLightBuffer;
	VkDeviceMemory directionalLightBufferMemory;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	std::vector<Texture*> textures;

	VkPipeline pipeline;
	VkPipeline pipeline2;

	VkPipeline computePipeline;

	VkPipelineLayout pipelineLayout;

	std::string vertexShaderPath;
	std::string fragmentShaderPath;

	std::string tessellationControlShaderPath;
	std::string tessellationEvaluationShaderPath;

	std::string geometryShaderPath;

	std::string computeShaderPath;

	VkRenderPass renderPass;

	std::vector<DirectionalLight>* pDirectionLights;


	float widthScale;
	float heightScale;

	glm::vec2 extent;

	bool vrMode;

	glm::ivec3 computeDispatchSize;

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

struct VoxelUniformBufferObject
{
	glm::mat4 mvpX;
	glm::mat4 mvpY;
	glm::mat4 mvpZ;
	int width;
	int height;
	int voxelSize;
	float halfVoxelSize;
};

struct VoxelFragCount {
	uint32_t voxelCount;
};

struct FragmentListData
{
	glm::vec4 data;
};

struct SVOInitInfo
{
	uint32_t voxelSize;
	uint32_t maxLevel;
	uint32_t currentIndex;
	uint32_t currentLevel;
};

struct SVONode
{
	uint32_t index;
	uint32_t parentIndex;
	uint32_t level;
	uint32_t fragListIndex;
	uint32_t fragListIndexArray[8];
	uint32_t childrenIndex[8];

	glm::vec4 pos;
	glm::vec4 albedo;

	//glm::vec4 padding[7];

	//glm::vec4 childrenPos[8];
	
};

struct SVOTagOutput
{
	uint32_t numChild;
	uint32_t parentIndex;
	uint32_t endMemoryIndex;	
	uint32_t padding;

	uint32_t childrenPlace[8];
	glm::vec4 childrenCenterPos[8];

	uint32_t levelOffset[12];
};

class VoxelizeMaterial : public Material
{
public:

	virtual ~VoxelizeMaterial()
	{
		cleanPipeline();
		cleanUp();

	}

	virtual void cleanUp()
	{
		vkDestroyBuffer(device, voxelUniformBuffer, nullptr);
		vkFreeMemory(device, voxelUniformBufferMemory, nullptr);
	}

	void createVoxelUniformBuffer(glm::mat4 model, glm::mat4 viewX, glm::mat4 viewY, glm::mat4 viewZ, glm::mat4 proj, int x, int y, int size, float halfVoxelSize)
	{
		createBuffer(sizeof(VoxelUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, voxelUniformBuffer, voxelUniformBufferMemory);

		VoxelUniformBufferObject vubo;

		vubo.mvpX = proj * viewX * model;
		vubo.mvpY = proj * viewY * model;
		vubo.mvpZ = proj * viewZ * model;
		vubo.width = x;
		vubo.height = y;
		vubo.voxelSize = size;
		vubo.halfVoxelSize = halfVoxelSize;
		void* data;
		vkMapMemory(device, voxelUniformBufferMemory, 0, sizeof(VoxelUniformBufferObject), 0, &data);
		memcpy(data, &vubo, sizeof(VoxelUniformBufferObject));
		vkUnmapMemory(device, voxelUniformBufferMemory);
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent);

	void setImages(VkImageView outputImageViewParam, VkImageView voxelAlbedoImageViewParam)
	{
		outputImageView = outputImageViewParam;
		//voxelPosImageView = voxelPosImageViewParam;
		voxelAlbedoImageView = voxelAlbedoImageViewParam;

	
	}
	void set3DImages(VkImageView albedo3DImageViewParam)
	{
		albedo3DImageView = albedo3DImageViewParam;
	}

	

	void setBuffers(VkBuffer voxelFragCountBufferParam, VkBuffer ouputPosListBufferParam, VkBuffer ouputAlbedoListBufferParam)
	{
		voxelFragCountBuffer = voxelFragCountBufferParam;
		ouputPosListBuffer = ouputPosListBufferParam;
		ouputAlbedoListBuffer = ouputAlbedoListBufferParam;

	}

	/*
	void createUniformBuffer()
	{
		createBuffer(sizeof(VoxelFragCount), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, voxelFragCountBuffer, voxelFragCountBufferMemory);
		createBuffer(sizeof(FragmentListData) * fragCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ouputPosListBuffer, ouputPosListBufferMemory);
		createBuffer(sizeof(FragmentListData) * fragCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ouputAlbedoListBuffer, ouputAlbedoListBufferMemory);
	}
	*/

	VkImageView albedo3DImageView;


	VkImageView outputImageView;
	//VkImageView voxelPosImageView;
	VkImageView voxelAlbedoImageView;

	VkBuffer voxelUniformBuffer;
	VkDeviceMemory voxelUniformBufferMemory;


	VkBuffer voxelFragCountBuffer;
	VkDeviceMemory voxelFragCountBufferMemory;

	VkBuffer ouputPosListBuffer;
	VkDeviceMemory ouputPosListBufferMemory;

	VkBuffer ouputAlbedoListBuffer;
	VkDeviceMemory ouputAlbedoListBufferMemory;

	uint32_t fragCount;
private:

};



class VoxelTagMaterial : public Material
{
public:

	virtual ~VoxelTagMaterial()
	{
		cleanPipeline();
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	void createComputePipeline();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}		

	void setBuffers(VkBuffer voxelFragCountBufferParam, VkBuffer voxelNodePoolParam, VkBuffer voxelPosBufferParam, VkBuffer SVOInitInfoBufferParam, VkBuffer voxelAlbedoBufferParam)
	{
		voxelFragCountBuffer = voxelFragCountBufferParam;
		voxelOctreeBuffer = voxelNodePoolParam;
		voxelPosBuffer = voxelPosBufferParam;
		SVOInitInfoBuffer = SVOInitInfoBufferParam;
		voxelAlbedoBuffer = voxelAlbedoBufferParam;
	}

	void createBuffers()
	{
		createBuffer(sizeof(SVOTagOutput) , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, NumChildBuffer, NumChildMemory);

	}

	virtual void cleanUp()
	{
		vkDestroyBuffer(device, NumChildBuffer, nullptr);
		vkFreeMemory(device, NumChildMemory, nullptr);
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}


	VkBuffer voxelFragCountBuffer;
	VkBuffer SVOInitInfoBuffer;
	VkBuffer voxelPosBuffer;
	VkBuffer voxelAlbedoBuffer;
	VkBuffer voxelOctreeBuffer;

	VkBuffer NumChildBuffer;
	VkDeviceMemory NumChildMemory;

	uint32_t maxNode;
	uint32_t fragCount;
private:

};

class VoxelAllocationMaterial : public Material
{
public:

	virtual ~VoxelAllocationMaterial()
	{
		cleanPipeline();
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	void createComputePipeline();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}

	void setBuffers(VkBuffer voxelFragCountBufferParam, VkBuffer voxelNodePoolParam, VkBuffer voxelPosBufferParam, VkBuffer SVOInitInfoBufferParam, VkBuffer NumChildBufferParam)
	{
		voxelFragCountBuffer = voxelFragCountBufferParam;
		voxelOctreeBuffer = voxelNodePoolParam;
		voxelPosBuffer = voxelPosBufferParam;
		SVOInitInfoBuffer = SVOInitInfoBufferParam;
		NumChildBuffer = NumChildBufferParam;
	}


	virtual void cleanUp()
	{
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}


	VkBuffer voxelFragCountBuffer;
	VkBuffer SVOInitInfoBuffer;
	VkBuffer voxelPosBuffer;
	VkBuffer voxelOctreeBuffer;

	VkBuffer NumChildBuffer;

	uint32_t maxNode;
	uint32_t fragCount;
private:

};

class VoxelMipmapMaterial : public Material
{
public:

	virtual ~VoxelMipmapMaterial()
	{
		cleanPipeline();
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	void createComputePipeline();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}

	void setBuffers(VkBuffer voxelFragCountBufferParam, VkBuffer voxelNodePoolParam, VkBuffer voxelPosBufferParam, VkBuffer SVOInitInfoBufferParam, VkBuffer NumChildBufferParam,
		VkBuffer voxelAlbedoBufferParam)
	{
		voxelFragCountBuffer = voxelFragCountBufferParam;
		voxelOctreeBuffer = voxelNodePoolParam;
		voxelPosBuffer = voxelPosBufferParam;
		SVOInitInfoBuffer = SVOInitInfoBufferParam;
		NumChildBuffer = NumChildBufferParam;
		voxelAlbedoBuffer = voxelAlbedoBufferParam;
	}


	virtual void cleanUp()
	{
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}


	VkBuffer voxelFragCountBuffer;
	VkBuffer SVOInitInfoBuffer;
	VkBuffer voxelPosBuffer;
	VkBuffer voxelOctreeBuffer;
	VkBuffer voxelAlbedoBuffer;
	VkBuffer NumChildBuffer;

	uint32_t maxNode;
	uint32_t fragCount;
private:

};

class VoxelTextureMaterial : public Material
{
public:

	virtual ~VoxelTextureMaterial()
	{
		cleanPipeline();
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	void createComputePipeline();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}

	void setBuffers(VkBuffer SVOInitInfoBufferParam)
	{
		SVOInitInfoBuffer = SVOInitInfoBufferParam;
	}

	void setImageviews(VkImageView upperalbedo3DImageViewParam, VkImageView loweralbedo3DImageViewParam)
	{
		upperalbedo3DImageView = upperalbedo3DImageViewParam;
		loweralbedo3DImageView = loweralbedo3DImageViewParam;
	}


	virtual void cleanUp()
	{
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}	
	VkBuffer SVOInitInfoBuffer;

	VkImageView upperalbedo3DImageView;
	VkImageView loweralbedo3DImageView;

private:

};

struct BoundBox
{
	glm::vec4 bounds[2]; //min and max
};

struct OctreeNode
{
	uint32_t index;
	uint32_t parentIndex;
	uint32_t level;
	uint32_t workGroupID[3];
	uint32_t childrenIndex[8]; //16 
	uint32_t padding[2];

	BoundBox bb;

	glm::vec4 albedo;
};


struct MemoryIndicator
{
	uint32_t endMemoryIndex;
};

class VoxelOctreeMaterial : public Material
{
public:

	virtual ~VoxelOctreeMaterial()
	{
		cleanPipeline();
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();
	void createComputePipeline();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}

	void setBuffers(VkBuffer SVOInitInfoBufferParam, VkBuffer OCtreeBufferParam, VkBuffer MemIndcBufferParam)
	{
		SVOInitInfoBuffer = SVOInitInfoBufferParam;
		OCtreeBuffer = OCtreeBufferParam;
		MemIndcBuffer = MemIndcBufferParam;

	}

	void setImageviews(VkImageView albedo3DImageViewParam)
	{
		albedo3DImageView = albedo3DImageViewParam;
	}


	virtual void cleanUp()
	{
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}

	VkBuffer SVOInitInfoBuffer;
	VkBuffer OCtreeBuffer;
	VkBuffer MemIndcBuffer;

	VkImageView albedo3DImageView;


	uint32_t numOCtreeNode;
private:

};



struct VoxelInfo
{
	glm::vec3 centerPos;
	float maxWidth;
	unsigned int voxelSize;
	unsigned int halfVoxelSize;
	unsigned int maxLevel;
	float standardObjScale;
};

class VoxelRenderMaterial : public Material
{
public:

	virtual ~VoxelRenderMaterial()
	{		
		cleanUp();

	}

	virtual void cleanUp()
	{
		
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	

	void setfragListVoxelCount(unsigned int size)
	{
		fragListVoxelCount = size;
	}

	void setImageViews(VkImageView pImageViews)
	{
		ImageViews = pImageViews;
	}

	void setBuffer(VkBuffer ouputPosListBufferParam, VkBuffer ouputAlbedoListBufferParam, VkBuffer voxelOctreeBufferParam, VkBuffer voxelInfoBufferParam)
	{
		ouputPosListBuffer = ouputPosListBufferParam;
		ouputAlbedoListBuffer = ouputAlbedoListBufferParam;
		voxelOctreeBuffer = voxelOctreeBufferParam;
		voxelInfoBuffer = voxelInfoBufferParam;
	}

	void setImageviews(VkImageView albedo3DImageViewParam)
	{
		albedo3DImageView = albedo3DImageViewParam;
	}


	void createVertexBuffer()
	{
		std::vector<SimpleVertex> vertices;
		vertices.resize(fragListVoxelCount);

		VkDeviceSize bufferSize = sizeof(SimpleVertex) *vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	unsigned int fragListVoxelCount;

	VkImageView ImageViews;

	VkBuffer voxelInfoBuffer;

	VkBuffer ouputPosListBuffer;

	VkBuffer ouputAlbedoListBuffer;

	VkBuffer voxelOctreeBuffer;

	VkImageView albedo3DImageView;

	uint32_t maxNode;
private:

};

class VoxelConetracingMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	void setImageViews(VkImageView outputImageViewsParam, VkImageView depthImageViewParam, VkImageView  normalImageViewParam, VkImageView  specularMapViewParam, std::vector<VkImageView> *albedo3DImageViewSetParam, VkImageView shadowMapViewParam)
	{
		ImageViews = outputImageViewsParam;
		depthImageView = depthImageViewParam;
		normalImageView = normalImageViewParam;
		specularMapView = specularMapViewParam;
		//albedo3DImageView = albedo3DImageViewParam;
		albedo3DImageViewSet = albedo3DImageViewSetParam;

		shadowMapView = shadowMapViewParam;
	}

	void setBuffers( VkBuffer VoxelInfoBufferParam, VkBuffer shadowConstantBufferParam)
	{
		VoxelInfoBuffer = VoxelInfoBufferParam;
		shadowConstantBuffer = shadowConstantBufferParam;
	}

	VkImageView ImageViews;
	VkImageView depthImageView;
	VkImageView normalImageView;
	VkImageView specularMapView;
	VkImageView shadowMapView;

	std::vector<VkImageView> *albedo3DImageViewSet;
	//VkImageView albedo3DImageView;

	//VkBuffer OctreeBuffer;
	VkBuffer VoxelInfoBuffer;
	VkBuffer shadowConstantBuffer;
	//uint32_t NumOctreeNode;

private:

};



class LightingMaterial : public Material
{
public:

	virtual ~LightingMaterial()
	{
		cleanUp();

	}

	virtual void cleanUp()
	{
		vkDestroyBuffer(device, shadowConstantBuffer, nullptr);
		vkFreeMemory(device, shadowConstantBufferMemory, nullptr);

		vkDestroyBuffer(device, optionBuffer, nullptr);
		vkFreeMemory(device, optionBufferMemory, nullptr);
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	virtual void createGraphicsPipeline(VkExtent2D swapChainExtent);

	void setGbuffers(std::vector<VkImageView>*  pGBufferImageViews, VkImageView pDepthImageView, VkImageView shadowMapViewParam, VkImageView GIViewParam)
	{
		gBufferImageViews = pGBufferImageViews;
		DepthImageView = pDepthImageView;
		shadowMapView = shadowMapViewParam;
		GIView = GIViewParam;
	}

	virtual void createUniformBuffer()
	{
		Material::createUniformBuffer();
		createBuffer(sizeof(ShadowUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shadowConstantBuffer, shadowConstantBufferMemory);
	
		createBuffer(sizeof(uint32_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, optionBuffer, optionBufferMemory);

	}

	std::vector<VkImageView>*  gBufferImageViews;
	VkImageView DepthImageView;

	VkImageView shadowMapView;

	VkBuffer shadowConstantBuffer;
	VkDeviceMemory shadowConstantBufferMemory;

	VkBuffer optionBuffer;
	VkDeviceMemory optionBufferMemory;

	VkImageView GIView;
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

	void setDubugBuffers(std::vector<VkImageView>*  pGBufferImageViews, VkImageView DepthImageViewParam, VkImageView GIImageViewParam, VkImageView shadowImageViewParam)
	{
		gBufferImageViews = pGBufferImageViews;
		GIImageView = GIImageViewParam;
		DepthImageView = DepthImageViewParam;
		shadowImageView = shadowImageViewParam;
	}

	std::vector<VkImageView>*  gBufferImageViews;

	VkImageView GIImageView;
	VkImageView shadowImageView;

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
		finalImageView = pImageView;
		DepthImageView = pDepthImageView;
	}

	VkImageView finalImageView;
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

class BlurMaterial : public Material
{
public:

	virtual ~BlurMaterial()
	{
		cleanUp();
	}

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

	virtual void createUniformBuffer()
	{
		Material::createUniformBuffer();

		createBuffer(sizeof(BlurUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, blurUniformBuffer, blurUniformBufferMemory);
	}

	virtual void cleanUp()
	{
		vkDestroyBuffer(device, blurUniformBuffer, nullptr);
		vkFreeMemory(device, blurUniformBufferMemory, nullptr);
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}

	VkImageView ImageViews;
	VkImageView DepthImageView;

	VkBuffer blurUniformBuffer;
	VkDeviceMemory blurUniformBufferMemory;

private:

};

class ComputeBlurMaterial : public Material
{
public:

	virtual ~ComputeBlurMaterial()
	{
		cleanUp();
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void updateDispatchSize(glm::ivec3 dispatchSize)
	{
		computeDispatchSize = dispatchSize;
	}

	

	void createComputePipeline(/*glm::vec2 Extent, glm::vec2 ScreenOffset*/);

	void setImageViews(VkImageView pInputImageView, VkImageView pOutputImageView)
	{
		inputImageViews = pInputImageView;
		outputImageViews = pOutputImageView;
	}

	virtual void createUniformBuffer()
	{
		Material::createUniformBuffer();

		createBuffer(sizeof(BlurUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, blurUniformBuffer, blurUniformBufferMemory);
	}

	virtual void cleanUp()
	{
		vkDestroyBuffer(device, blurUniformBuffer, nullptr);
		vkFreeMemory(device, blurUniformBufferMemory, nullptr);
	}

	virtual void cleanPipeline()
	{
		Material::cleanPipeline();
	}
	
	VkImageView inputImageViews;
	VkImageView outputImageViews;
	VkBuffer blurUniformBuffer;
	VkDeviceMemory blurUniformBufferMemory;
private:

};


class LastPostProcessgMaterial : public Material
{
public:

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	void setImageViews(VkImageView pImageView, VkImageView pBloomImageView, VkImageView pDepthImageView)
	{
		sceneImageView = pImageView;
		bloomImageView = pBloomImageView;
		DepthImageView = pDepthImageView;
	}

	VkImageView sceneImageView;
	VkImageView bloomImageView;
	VkImageView DepthImageView;

private:

};


class StandardShadowMaterial : public Material
{
public:

	virtual ~StandardShadowMaterial()
	{
		//cleanPipeline();
		cleanUp();

	}

	virtual void cleanUp()
	{
		//vkDestroyBuffer(device, ShadowConstantBuffer, nullptr);
		//vkFreeMemory(device, ShadowConstantBufferMemory, nullptr);

		vkDestroyBuffer(device, objectUniformBuffer, nullptr);
		vkFreeMemory(device, objectUniformMemory, nullptr);
	}

	virtual void createDescriptorSetLayout();
	virtual void createDescriptorPool();
	virtual void createDescriptorSet();

	void updateDescriptorSet();

	void createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset);

	virtual void createUniformBuffer()
	{
		//createBuffer(sizeof(ShadowUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ShadowConstantBuffer, ShadowConstantBufferMemory);
		createBuffer(sizeof(ObjectUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, objectUniformBuffer, objectUniformMemory);
	}

	void setUniformBuffer(VkBuffer ShadowConstantBufferParam)
	{
		ShadowConstantBuffer = ShadowConstantBufferParam;
	}

	VkBuffer ShadowConstantBuffer;
	//VkDeviceMemory ShadowConstantBufferMemory;

	VkBuffer objectUniformBuffer;
	VkDeviceMemory objectUniformMemory;

private:

};


