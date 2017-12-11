#pragma once



#include "Asset.h"

class Texture : public Asset
{
public:
	Texture():mipLevel(0)
	{

	}

	~Texture()
	{
		cleanUp();
	}

	void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam);
	void createTextureImage(std::string path);

	void setMiplevel(int mipLevelParam)
	{
		mipLevel = mipLevelParam;
	}

	VkImageView createImageView(VkImage image, VkFormat format);
	void createTextureImageView(VkFormat format);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory,
		int mipLevelParam);

	void createTextureSampler();

	virtual void cleanUp();

	VkImageView textureImageView;
	VkSampler textureSampler;

	int mipLevel;

	

private:
	int texWidth, texHeight, texChannels;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	
};

