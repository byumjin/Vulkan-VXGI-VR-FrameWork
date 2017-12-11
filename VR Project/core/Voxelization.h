#pragma once

#include "Common.h"
#include "VulkanQueue.h"
#include "../actors/Camera.h"
#include "../actors/Object.h"

#define VOXEL_SIZE 512


class Voxelization
{
public:
	~Voxelization()
	{
		//voxelizeMaterial->cleanPipeline();		

		
	}

	void shutDown()
	{
		for (size_t i = 0; i < VXGIMaterials.size(); i++)
		{
			delete VXGIMaterials[i];
		}

		if (VXGITagMaterial != NULL)
			delete VXGITagMaterial;
		if (VXGIAllocMaterial != NULL)
			delete VXGIAllocMaterial;
		if (VXGIMipmapMaterial != NULL)
			delete VXGIMipmapMaterial;

		if (VXGITextureMaterial != NULL)
			delete VXGITextureMaterial;

		if (fragListCommandBuffer != NULL)
			vkFreeCommandBuffers(device, commandPool, 1, &fragListCommandBuffer);

		if (octreeCommandBuffer != NULL)
			vkFreeCommandBuffers(device, commandPool, 1, &octreeCommandBuffer);

		if (allocateCommandBuffer != NULL)
			vkFreeCommandBuffers(device, commandPool, 1, &allocateCommandBuffer);

		if (mipMapCommandBuffer != NULL)
			vkFreeCommandBuffers(device, commandPool, 1, &mipMapCommandBuffer);

		if (textureCommandBuffer != NULL)
			vkFreeCommandBuffers(device, commandPool, 1, &textureCommandBuffer);

		vkDestroySemaphore(device, voxelSemaphore, nullptr);
		vkDestroySemaphore(device, voxelMipmapSemaphore, nullptr);
		vkDestroySemaphore(device, voxelOctreeSemaphoreA, nullptr);
		vkDestroySemaphore(device, voxelOctreeSemaphoreB, nullptr);
		cleanUp();


		//vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
	}

	void cleanUp()
	{
		for (uint32_t i = 0; i <= miplevel; i++)
		{
			vkFreeMemory(device, albedo3DImageMemorySet[i], nullptr);
			vkDestroyImageView(device, albedo3DImageViewSet[i], nullptr);
			vkDestroyImage(device, albedo3DImageSet[i], nullptr);
		}

		vkDestroyBuffer(device, voxelInfoBuffer, nullptr);
		vkFreeMemory(device, voxelInfoBufferMemory, nullptr);

		vkFreeMemory(device, outputImageMemory, nullptr);
		vkDestroyImageView(device, outputImageView, nullptr);
		vkDestroyImage(device, outputImage, nullptr);

		vkDestroyBuffer(device, voxelFragCountBuffer, nullptr);
		vkFreeMemory(device, voxelFragCountBufferMemory, nullptr);
		vkDestroyBuffer(device, ouputPosListBuffer, nullptr);
		vkFreeMemory(device, ouputPosListBufferMemory, nullptr);
		vkDestroyBuffer(device, ouputAlbedoListBuffer, nullptr);
		vkFreeMemory(device, ouputAlbedoListBufferMemory, nullptr);

		vkDestroyBuffer(device, OctreeBuffer, nullptr);
		vkFreeMemory(device, OctreeMemory, nullptr);

		vkDestroyBuffer(device, OctreeMemIndcBuffer, nullptr);
		vkFreeMemory(device, OctreeMemIndcMemory, nullptr);
		
		
		vkDestroyBuffer(device, SVOInitInfoBuffer, nullptr);
		vkFreeMemory(device, SVOInitInfoMemory, nullptr);
	}

	void release3dImages()
	{
		
	}

	VkSemaphore createVoxels(const Camera &camera, VkSemaphore semaphoreParam)
	{
		//voxelization
		UniformBufferObject ubo = {};

		ubo.viewMat = camera.viewMat;
		ubo.projMat = camera.projMat;
		ubo.viewProjMat = camera.viewProjMat;
		ubo.InvViewProjMat = camera.InvViewProjMat;
		ubo.modelViewProjMat = ubo.viewProjMat;
		ubo.cameraWorldPos = camera.position;

		ubo.modelMat = standardObject->modelMat;
		ubo.modelViewProjMat = ubo.viewProjMat * ubo.modelMat;

		glm::mat4 A = ubo.modelMat;
		A[3] = glm::vec4(0, 0, 0, 1);
		ubo.InvTransposeMat = glm::transpose(glm::inverse(A));

		void* data;
		vkMapMemory(device, VXGIMaterials[0]->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &ubo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, VXGIMaterials[0]->uniformBufferMemory);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		if (semaphoreParam == VK_NULL_HANDLE) {
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
		} else {
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &semaphoreParam;
		}
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &voxelSemaphore;

		if (vkQueueSubmit(VXGIMaterials[0]->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		

		vkQueueWaitIdle(VXGIMaterials[0]->queue);

		//return voxelSemaphore;
		return voxelSemaphore;
	}

	VkSemaphore createMipmaps(VkSemaphore semaphoreParam)
	{
		VkSemaphore prevSemaphore = semaphoreParam;
		VkSemaphore currentSemaphore;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		SVOInitInfo svoInfo = {};

		svoInfo.voxelSize = VOXEL_SIZE;
		svoInfo.maxLevel = miplevel;
		svoInfo.currentIndex = 0;

		//create mipmaps
		for (size_t t = 0; t < 5 /*miplevel*/; t++)
		{
			initTextureMaterial();

			svoInfo.currentLevel = static_cast<uint32_t>(t);

			void* data;
			vkMapMemory(device, SVOInitInfoMemory, 0, sizeof(SVOInitInfo), 0, &data);
			memcpy(data, &svoInfo, sizeof(SVOInitInfo));
			vkUnmapMemory(device, SVOInitInfoMemory);


			VXGITextureMaterial->setBuffers(SVOInitInfoBuffer);
			VXGITextureMaterial->setImageviews(albedo3DImageViewSet[t], albedo3DImageViewSet[t + 1]);
			VXGITextureMaterial->createDescriptorSet();

			uint32_t workGroupSize;

			workGroupSize = svoInfo.voxelSize / static_cast<uint32_t>(pow(2, t));

			VXGITextureMaterial->updateDispatchSize(glm::ivec3(workGroupSize, workGroupSize, workGroupSize));
			VXGITextureMaterial->createComputePipeline();

			createTextureCommandBuffers();

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &prevSemaphore;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &textureCommandBuffer;

			if (prevSemaphore != voxelOctreeSemaphoreA)
				currentSemaphore = voxelOctreeSemaphoreA;
			else
				currentSemaphore = voxelOctreeSemaphoreB;

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &currentSemaphore;

			if (vkQueueSubmit(VXGITextureMaterial->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to submit draw command buffer!");
			}

			vkQueueWaitIdle(VXGITextureMaterial->queue);

			prevSemaphore = currentSemaphore;

			vkFreeCommandBuffers(device, commandPool, 1, &textureCommandBuffer);
			textureCommandBuffer = NULL;

			delete VXGITextureMaterial;
			VXGITextureMaterial = NULL;
		}

		return prevSemaphore;
	}

	void Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam, int LayerCount, uint32_t miplevelParam, glm::vec2 Scales);

	void createImage(uint32_t width, uint32_t height , uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits countBits, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createImages();

	void createImages(VkFormat formatParam, VkImageTiling tilingParam, VkMemoryPropertyFlags propertiesParam);

	void createMipImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits countBits, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	/*
	void createImages(uint32_t maxWidth)
	{
		createImage(maxWidth, maxWidth, maxWidth, VK_IMAGE_TYPE_3D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			albedo3DImage, albedo3DImageMemory);

		albedo3DImageView = createImageView(albedo3DImage, VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);


	}
	*/

	void createOctree()
	{
		std::vector<OctreeNode> OctreeVec;
		OctreeVec.resize(maxiumOCtreeNodeCount);


		OctreeNode rootNode = {};

		rootNode.index = 0;
		rootNode.level = 0;
		rootNode.parentIndex = 0;
		rootNode.workGroupID[0] = 0;
		rootNode.workGroupID[1] = 0;
		rootNode.workGroupID[2] = 0;
		rootNode.albedo = glm::vec4(1.0);

		rootNode.bb.bounds[0] = -glm::vec4(float(VOXEL_SIZE / 2));
		rootNode.bb.bounds[1] = glm::vec4(float(VOXEL_SIZE / 2));

		void *data2;

		vkMapMemory(device, OctreeMemory, 0, sizeof(OctreeNode), 0, &data2);
		memcpy(data2, &rootNode, sizeof(OctreeNode));
		vkUnmapMemory(device, OctreeMemory);

		MemoryIndicator memindc = {};
		memindc.endMemoryIndex = 1;

		vkMapMemory(device, OctreeMemIndcMemory, 0, sizeof(MemoryIndicator), 0, &data2);
		memcpy(data2, &memindc, sizeof(MemoryIndicator));
		vkUnmapMemory(device, OctreeMemIndcMemory);

		uint32_t generatedNewNodeCount = memindc.endMemoryIndex;
		uint32_t prevendMemoryIndex = 0;
		/*
		//createOctree
		for (size_t t = 1; t <= miplevel; t++)
		{
			initOCtreeMaterial();

			svoInfo.voxelSize = VOXEL_SIZE;
			svoInfo.maxLevel = voxelizator.miplevel;
			svoInfo.currentIndex = generatedNewNodeCount;
			//forInitIndex
			svoInfo.currentLevel = prevendMemoryIndex;

			vkMapMemory(device, voxelizator.SVOInitInfoMemory, 0, sizeof(SVOInitInfo), 0, &data2);
			memcpy(data2, &svoInfo, sizeof(SVOInitInfo));
			vkUnmapMemory(device, voxelizator.SVOInitInfoMemory);

			prevendMemoryIndex = memindc.endMemoryIndex;

			voxelizator.VXGIOctreeMaterial->setBuffers(voxelizator.SVOInitInfoBuffer, voxelizator.OctreeBuffer, voxelizator.OctreeMemIndcBuffer);
			voxelizator.VXGIOctreeMaterial->setImageviews(voxelizator.albedo3DImageViewSet[t]);
			voxelizator.VXGIOctreeMaterial->createDescriptorSet();


			uint32_t workGroupSize;
			workGroupSize = (uint32_t)(ceil((float)generatedNewNodeCount / 1024.0));

			voxelizator.VXGIOctreeMaterial->updateDispatchSize(glm::ivec3(workGroupSize, 1, 1));
			voxelizator.VXGIOctreeMaterial->createComputePipeline();

			voxelizator.createOctreeCommandBuffers();

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &prevSemaphore;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &voxelizator.octreeCommandBuffer;

			if (prevSemaphore != voxelizator.voxelOctreeSemaphoreA)
				currentSemaphore = voxelizator.voxelOctreeSemaphoreA;
			else
				currentSemaphore = voxelizator.voxelOctreeSemaphoreB;

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &currentSemaphore;

			if (vkQueueSubmit(voxelizator.VXGIOctreeMaterial->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to submit draw command buffer!");
			}

			vkQueueWaitIdle(voxelizator.VXGIOctreeMaterial->queue);

			prevSemaphore = currentSemaphore;


			vkFreeCommandBuffers(device, voxelizator.commandPool, 1, &voxelizator.octreeCommandBuffer);
			voxelizator.octreeCommandBuffer = NULL;

			delete voxelizator.VXGIOctreeMaterial;
			voxelizator.VXGIOctreeMaterial = NULL;

			vkMapMemory(device, voxelizator.OctreeMemIndcMemory, 0, sizeof(MemoryIndicator), 0, &data2);
			memcpy(&memindc, data2, sizeof(MemoryIndicator));
			vkUnmapMemory(device, voxelizator.OctreeMemIndcMemory);

			generatedNewNodeCount = memindc.endMemoryIndex - generatedNewNodeCount;
		}
		*/
	}

	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags);
	VkImageView createResourceImageView(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags);

	void createFramebuffer();
	void createRenderPass();
	void createCommandPool();
	void createCommandBuffers();

	void draw();

	void createSemaphore()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &voxelSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}


		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &voxelMipmapSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &voxelOctreeSemaphoreA) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &voxelOctreeSemaphoreB) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}

	glm::vec2 getImageSize()
	{
		return glm::vec2(extent2D.width, extent2D.height);
	}

	void setQueue(VkQueue queueParam, VkQueue TagQueueParam, VkQueue AllocationParam, VkQueue MipmapQueueParam)
	{
		queue = queueParam;
		TagQueue = TagQueueParam;
		AllocationQueue = AllocationParam;
		MipmapQueue = MipmapQueueParam;
	}

	void initMaterial()
	{	
		for (size_t i = 0; i < standardObject->geos.size() ; i++)
		{
			VoxelizeMaterial *voxelizeMaterial = new VoxelizeMaterial;
			voxelizeMaterial->LoadFromFilename(device, physicalDevice, commandPool, queue, "voxel_material");

			voxelizeMaterial->addTexture(standardObject->materials[i]->textures[0]);
			voxelizeMaterial->addTexture(standardObject->materials[i]->textures[1]);
			voxelizeMaterial->addTexture(standardObject->materials[i]->textures[2]);
			voxelizeMaterial->addTexture(standardObject->materials[i]->textures[3]);

			voxelizeMaterial->setBuffers(voxelFragCountBuffer, ouputPosListBuffer, ouputAlbedoListBuffer);

			voxelizeMaterial->setShaderPaths("shaders/voxelization.vert.spv", "shaders/voxelization.frag.spv", "", "", "shaders/voxelization.geom.spv", "");
			voxelizeMaterial->createVoxelUniformBuffer(glm::mat4(1.0), viewX, viewY, viewZ, proj, extent2D.width, extent2D.height, VOXEL_SIZE, halfVoxelSize);

			VXGIMaterials.push_back(voxelizeMaterial);
		}		
	}



	void setMatrices()
	{
		float maxLen = glm::max(glm::max(standardObject->AABB.Extents.x, standardObject->AABB.Extents.y), standardObject->AABB.Extents.z);
		float maxLen2 = maxLen * 2.0f;

		halfVoxelSize = maxLen * 0.5f;

		proj = glm::orthoRH(-maxLen, maxLen, -maxLen, maxLen, 0.0f, maxLen2);
		proj[1][1] *= -1;

		viewX = glm::lookAt(standardObject->AABB.Center - glm::vec3(maxLen, 0.0f, 0.0f), standardObject->AABB.Center, glm::vec3(0.0f, 1.0f, 0.0f));
		viewY = glm::lookAt(standardObject->AABB.Center - glm::vec3(0.0f, maxLen, 0.0f), standardObject->AABB.Center, glm::vec3(-1.0f, 0.0f, 0.0f));
		viewZ = glm::lookAt(standardObject->AABB.Center - glm::vec3(0.0f, 0.0f, maxLen), standardObject->AABB.Center, glm::vec3(0.0f, 1.0f, 0.0f));

	}



	//!!!!!!DELETE LATER
	void createBuffers(uint32_t fragCountParam)
	{
		fragCount = fragCountParam;

		createBuffer(sizeof(VoxelFragCount), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, voxelFragCountBuffer, voxelFragCountBufferMemory);
		createBuffer(sizeof(FragmentListData) * fragCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ouputPosListBuffer, ouputPosListBufferMemory);
		createBuffer(sizeof(FragmentListData) * fragCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ouputAlbedoListBuffer, ouputAlbedoListBufferMemory);
	}
	
	

	void createFragListImages();

	
	void createOctreeCommandBuffers();

	void createAllocateCommandBuffers();


	void createMipmapCommandBuffers();

	void createTextureCommandBuffers();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}


	void initTagMaterial()
	{
		VXGITagMaterial = new VoxelTagMaterial;
		VXGITagMaterial->LoadFromFilename(device, physicalDevice, commandPool, TagQueue, "voxel_tag_material");
		VXGITagMaterial->setShaderPaths("", "", "", "", "", "shaders/voxelTag.comp.spv");
	}


	void initAllocMaterial()
	{
		VXGIAllocMaterial = new VoxelAllocationMaterial;
		VXGIAllocMaterial->LoadFromFilename(device, physicalDevice, commandPool, AllocationQueue, "voxel_alloc_material");
		VXGIAllocMaterial->setShaderPaths("", "", "", "", "", "shaders/voxelAlloc.comp.spv");
	}

	void initMipmapMaterial()
	{
		VXGIMipmapMaterial = new VoxelMipmapMaterial;
		VXGIMipmapMaterial->LoadFromFilename(device, physicalDevice, commandPool, MipmapQueue, "voxel_mipmap_material");
		VXGIMipmapMaterial->setShaderPaths("", "", "", "", "", "shaders/voxelMipmap.comp.spv");
	}
	

	void initTextureMaterial()
	{
		VXGITextureMaterial = new VoxelTextureMaterial;
		VXGITextureMaterial->LoadFromFilename(device, physicalDevice, commandPool, MipmapQueue, "voxel_texture_material");
		VXGITextureMaterial->setShaderPaths("", "", "", "", "", "shaders/voxel3DTexture.comp.spv");
	}

	void initOCtreeMaterial()
	{
		VXGIOctreeMaterial = new VoxelOctreeMaterial;
		VXGIOctreeMaterial->numOCtreeNode = maxiumOCtreeNodeCount;
		VXGIOctreeMaterial->LoadFromFilename(device, physicalDevice, commandPool, TagQueue, "voxel_texture_material");
		VXGIOctreeMaterial->setShaderPaths("", "", "", "", "", "shaders/voxelOctree.comp.spv");
	}

	void createOctreeBuffer(uint32_t param)
	{
		maxiumOCtreeNodeCount = param;
		createBuffer(sizeof(OctreeNode) * maxiumOCtreeNodeCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, OctreeBuffer, OctreeMemory);
	
		createBuffer(sizeof(MemoryIndicator), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, OctreeMemIndcBuffer, OctreeMemIndcMemory);

	}

	void createSVOInitInfoBuffer()
	{
		createBuffer(sizeof(SVOInitInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, SVOInitInfoBuffer, SVOInitInfoMemory);
	}

	void createVoxelInfoBuffer(glm::vec3 centerPosParam, float maxWidthParam, unsigned int voxelSizeParam, float scale)
	{
		createBuffer(sizeof(VoxelInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, voxelInfoBuffer, voxelInfoBufferMemory);

		VoxelInfo vubo;

		vubo.centerPos = centerPosParam;
		vubo.maxWidth = maxWidthParam;
		vubo.voxelSize = voxelSizeParam;
		vubo.halfVoxelSize = vubo.voxelSize / 2;

		vubo.maxLevel = (uint32_t)log2(voxelSizeParam);
		vubo.standardObjScale = scale;

		void* data;
		vkMapMemory(device, voxelInfoBufferMemory, 0, sizeof(VoxelInfo), 0, &data);
		memcpy(data, &vubo, sizeof(VoxelInfo));
		vkUnmapMemory(device, voxelInfoBufferMemory);
	}


	std::vector<VoxelizeMaterial*> VXGIMaterials;

	VoxelTagMaterial* VXGITagMaterial;
	VoxelAllocationMaterial* VXGIAllocMaterial;
	VoxelMipmapMaterial* VXGIMipmapMaterial;

	VoxelTextureMaterial* VXGITextureMaterial;
	VoxelOctreeMaterial* VXGIOctreeMaterial;

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;


	VkCommandBuffer fragListCommandBuffer;
	
	VkCommandBuffer allocateCommandBuffer;
	VkCommandBuffer mipMapCommandBuffer;

	VkCommandBuffer textureCommandBuffer;

	VkCommandBuffer octreeCommandBuffer;

	VkFramebuffer frameBuffer;

	
	VkQueue queue;


	VkQueue TagQueue;
	VkQueue AllocationQueue;
	VkQueue MipmapQueue;

	VkFormat format;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;

	VkImage outputImage;
	VkImageView outputImageView;
	VkDeviceMemory outputImageMemory;
	
	std::vector<VkImage> albedo3DImageSet;
	std::vector<VkImageView> albedo3DImageViewSet;
	std::vector<VkDeviceMemory> albedo3DImageMemorySet;


	VkBuffer voxelFragCountBuffer;
	VkDeviceMemory voxelFragCountBufferMemory;
	VkBuffer ouputPosListBuffer;
	VkDeviceMemory ouputPosListBufferMemory;
	VkBuffer ouputAlbedoListBuffer;
	VkDeviceMemory ouputAlbedoListBufferMemory;

	VkBuffer voxelInfoBuffer;
	VkDeviceMemory voxelInfoBufferMemory;

	uint32_t maxiumOCtreeNodeCount;
	uint32_t octreeLevel;
	VkBuffer OctreeBuffer;
	VkDeviceMemory OctreeMemory;

	VkBuffer OctreeMemIndcBuffer;
	VkDeviceMemory OctreeMemIndcMemory;


	VkBuffer SVOInitInfoBuffer;
	VkDeviceMemory SVOInitInfoMemory;
	
	
	uint32_t miplevel;

	uint32_t fragCount;

	VkExtent2D extent2D;
	int LayerCount;

	//Material* material;
	Object* standardObject;

	VkSemaphore voxelSemaphore;
	VkSemaphore voxelMipmapSemaphore;

	VkSemaphore voxelOctreeSemaphoreA;
	VkSemaphore voxelOctreeSemaphoreB;
	glm::mat4 viewX;
	glm::mat4 viewY;
	glm::mat4 viewZ;

	glm::mat4 proj;

	float halfVoxelSize;

};