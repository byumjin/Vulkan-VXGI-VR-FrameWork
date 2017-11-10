#pragma once

#include "Common.h"
#include "VulkanQueue.h"
#include "../assets/Geometry.h"
#include "../assets/Material.h"

class PostProcess
{
public:

	void cleanUp()
	{

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

	VkImage image;
	VkImageView imageView;
	VkDeviceMemory imageMemory;

	singleTriangular* offScreenPlane;


	VkExtent2D* pExtent2D;
	int LayerCount;

	Material* material;

};