#pragma once

#include "Common.h"
#include "VulkanQueue.h"
#include "../assets/Geometry.h"
#include "../assets/Material.h"

enum DrawMode
{
	DRAW_VERTEX = 0,
	DRAW_INDEX = 1,
	DRAW_DISPATCH = 2,
};

class PostProcess
{
public:
	PostProcess()
	{
		device = NULL;
		physicalDevice = NULL;
		surface = NULL;
		pExtent2D = NULL;
		LayerCount = 0;

		miplevel = 0;

		isCSPostProcess = false;

		bActivate = true;

		drawMode = -1;

		vertexSize = 0;
		NeedDepth = false;

		outputImage = NULL;
		outputImageView = NULL;
		outputImageMemory = NULL;

		depthImage = NULL;
		depthImageMemory = NULL;
		depthImageView = NULL;
	}

	~PostProcess()
	{
		vkDestroySemaphore(device, postProcessSemaphore, nullptr);
	}

	void cleanUp()
	{
		vkFreeMemory(device, outputImageMemory, nullptr);
		vkDestroyImageView(device, outputImageView, nullptr);
		vkDestroyImage(device, outputImage, nullptr);

		if (NeedDepth)
		{
			vkFreeMemory(device, depthImageMemory, nullptr);
			vkDestroyImageView(device, depthImageView, nullptr);
			vkDestroyImage(device, depthImage, nullptr);
		}
		
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat()
	{
		return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	void createDepthResources()
	{
		VkFormat depthFormat = findDepthFormat();

		if (bVRmode)
		{
			createImage(pExtent2D->width / 2, pExtent2D->height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
			depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		}
		else
		{
			createImage(pExtent2D->width, pExtent2D->height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
			depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		}

		transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, commandPool);
	}
	
	VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandPool, commandBuffer, material->queue);
	}

	


	bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}


	void Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam,
		VkExtent2D* extent2DParami, int LayerCount, uint32_t miplevelParam, glm::vec2 Scales, bool compute, unsigned int drawModeParam, uint32_t vertexSizeParam, bool bDepth, VkImageView depthImageParam);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createImages();

	void createImages(VkFormat formatParam, VkImageTiling tilingParam, VkImageUsageFlags usageParam, VkMemoryPropertyFlags propertiesParam);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);


	void createFramebuffer();
	void createRenderPass();
	void createCommandPool();
	void createCommandBuffers();

	void createSemaphore()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &postProcessSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}

	glm::vec2 getImageSize()
	{
		return glm::vec2(widthScale * pExtent2D->width, heightScale * pExtent2D->height);
	}

	glm::vec2 getScreenScale()
	{
		return glm::vec2(1.0/widthScale, 1.0/heightScale);
	}

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFramebuffer frameBuffer;

	VkQueue queue;

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

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	uint32_t miplevel;

	singleTriangular* offScreenPlane;


	VkExtent2D* pExtent2D;
	int LayerCount;

	Material* material;

	VkSemaphore postProcessSemaphore;

	

	bool NeedDepth;

	float widthScale;
	float heightScale;

	bool vrMode;
	bool isCSPostProcess;//Post Process For Compute Shader

	bool bActivate;

	unsigned int drawMode;
	uint32_t vertexSize;
};