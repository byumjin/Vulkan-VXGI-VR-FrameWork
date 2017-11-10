#pragma once



#include "Asset.h"

class Texture : public Asset
{
public:
	Texture()
	{

	}

	~Texture()
	{
		cleanUp();
	}

	void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam);
	void createTextureImage(std::string path);

	VkImageView createImageView(VkImage image, VkFormat format);
	void createTextureImageView(VkFormat format);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);	

	void createTextureSampler();

	virtual void cleanUp();

	VkImageView textureImageView;
	VkSampler textureSampler;

private:

	int texWidth, texHeight, texChannels;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
};

