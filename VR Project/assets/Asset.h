#pragma once

#include <string>
#include <tuple>

#include "../core/Common.h"

// Base class for assets like meshes, textures, shaders etc.
// Some of these assets will persist in memory as long as the user decides
class Asset
{
public:
	Asset();

	// Not all assets must implement this, but if you want it to be cached you must
	virtual void LoadFromFilename(std::string filename);
	virtual ~Asset();
	virtual void cleanUp() = 0;

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkCommandPool commandPool;
	VkQueue queue;

	std::string path;
};
