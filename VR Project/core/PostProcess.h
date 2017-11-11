#pragma once

#include "Common.h"
#include "VulkanQueue.h"
#include "../assets/Geometry.h"
#include "../assets/Material.h"

class PostProcess
{
public:
	~PostProcess()
	{
		vkDestroySemaphore(device, postProcessSemaphore, nullptr);
	}

	void cleanUp()
	{
		vkFreeMemory(device, outputImageMemory, nullptr);
		vkDestroyImageView(device, outputImageView, nullptr);
		vkDestroyImage(device, outputImage, nullptr);

		
	}

	void Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam, VkExtent2D* extent2DParami, int LayerCount);

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

	singleTriangular* offScreenPlane;


	VkExtent2D* pExtent2D;
	int LayerCount;

	Material* material;

	VkSemaphore postProcessSemaphore;

};