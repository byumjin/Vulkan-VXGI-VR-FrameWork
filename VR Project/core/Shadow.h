#pragma once

#include "Common.h"
#include "VulkanQueue.h"
#include "../actors/Object.h"

class StandardShadow
{
public:
	StandardShadow();
	~StandardShadow();

	void Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam, int LayerCountParam, VkQueue queueParam, std::vector<Object*> *pObjectManagerParam);

	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);	
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
	void endSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool);

	bool hasStencilComponent(VkFormat format);

	void createRenderPass();
	void createFramebuffer();	
	void createCommandPool();
	void createCommandBuffers();
	void createSemaphore();
	void createDepthResources();

	void createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits countBits, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImages(VkFormat formatParam, VkImageTiling tilingParam, VkMemoryPropertyFlags propertiesParam);
	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags);

	void setExtent(uint32_t width, uint32_t height)
	{
		Extent2D.width = width;
		Extent2D.height = height;
	}

	void cleanUp()
	{
		vkDestroyImage(device, outputImage, nullptr);
		vkDestroyImageView(device, outputImageView, nullptr);
		vkFreeMemory(device, outputImageMemory, nullptr);

		vkDestroyImage(device, depthImage, nullptr);
		vkDestroyImageView(device, depthImageView, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);		
	}

	void shutDown()
	{
		//cleanUp();
		vkDestroySemaphore(device, semaphore, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr);

		
	}

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFramebuffer frameBuffer;

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

	VkExtent2D Extent2D;
	int LayerCount;

	//StandardShadowMaterial* material;

	std::vector<Object*> *pObjectManager;

	VkSemaphore semaphore;
	VkQueue queue;
	float widthScale;
	float heightScale;


};
