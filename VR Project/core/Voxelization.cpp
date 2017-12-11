#include "Voxelization.h"

void Voxelization::Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam, int LayerCountParam, uint32_t miplevelParam, glm::vec2 Scales)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	surface = surfaceParam;
	extent2D.width = VOXEL_SIZE;
	extent2D.height = VOXEL_SIZE;
	LayerCount = LayerCountParam;
	miplevel = miplevelParam;

	createSVOInitInfoBuffer();


	octreeLevel = (uint32_t)log2(VOXEL_SIZE);

	
}

void Voxelization::createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits countBits, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = countBits;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void Voxelization::createMipImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits countBits, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels =  miplevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = countBits;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

uint32_t Voxelization::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Voxelization::createImages()
{
	createImage((uint32_t)(extent2D.width), (uint32_t)(extent2D.height), 1, VK_IMAGE_TYPE_2D, format, tiling, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_8_BIT, properties, outputImage, outputImageMemory);
	outputImageView = createImageView(outputImage, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT);

	for (uint32_t i = 0; i <= miplevel; i++)
	{
		uint32_t dimension = (uint32_t)pow(2, i);

		if (i == 0)
		{
			createMipImage(VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VK_IMAGE_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, properties, albedo3DImageSet[i], albedo3DImageMemorySet[i]);
			albedo3DImageViewSet[i] = createResourceImageView(albedo3DImageSet[i], VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
		{
			createImage(VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VK_IMAGE_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, properties, albedo3DImageSet[i], albedo3DImageMemorySet[i]);
			albedo3DImageViewSet[i] = createImageView(albedo3DImageSet[i], VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}
}


void Voxelization::createImages(VkFormat formatParam, VkImageTiling tilingParam, VkMemoryPropertyFlags propertiesParam)
{
	format = formatParam;
	tiling = tilingParam;
	properties = propertiesParam;

	createImage((uint32_t)(extent2D.width), (uint32_t)(extent2D.height), 1, VK_IMAGE_TYPE_2D, format, tiling, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_8_BIT, properties, outputImage, outputImageMemory);
	outputImageView = createImageView(outputImage, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT);

	albedo3DImageSet.resize(miplevel + 1);
	albedo3DImageViewSet.resize(miplevel + 1);
	albedo3DImageMemorySet.resize(miplevel + 1);

	for (uint32_t i = 0; i <= miplevel; i++)
	{
		uint32_t dimension = (uint32_t)pow(2, i);

		if (i == 0)
		{
			createMipImage(VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VK_IMAGE_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, properties, albedo3DImageSet[i], albedo3DImageMemorySet[i]);
			albedo3DImageViewSet[i] = createResourceImageView(albedo3DImageSet[i], VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
		{
			createImage(VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VOXEL_SIZE / dimension, VK_IMAGE_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, properties, albedo3DImageSet[i], albedo3DImageMemorySet[i]);
			albedo3DImageViewSet[i] = createImageView(albedo3DImageSet[i], VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		}		
	}
}

void Voxelization::createFragListImages()
{
// 	createImage((uint32_t)fragCount, (uint32_t)1, (uint32_t)1, VK_IMAGE_TYPE_1D, format, tiling, VK_IMAGE_USAGE_STORAGE_BIT, properties, ouputPosListImage, ouputPosListImageMemory);
// 	ouputPosListImageView = createImageView(ouputPosListImage, VK_IMAGE_VIEW_TYPE_1D, format, VK_IMAGE_ASPECT_COLOR_BIT);
// 
// 	createImage((uint32_t)fragCount, (uint32_t)1, (uint32_t)1, VK_IMAGE_TYPE_1D, format, tiling, VK_IMAGE_USAGE_STORAGE_BIT, properties, ouputAlbedoListImage, ouputAlbedoListImageMemory);
// 	ouputAlbedoListImageView = createImageView(ouputAlbedoListImage, VK_IMAGE_VIEW_TYPE_1D, format, VK_IMAGE_ASPECT_COLOR_BIT);




}

VkImageView Voxelization::createResourceImageView(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = type;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = miplevel;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkImageView Voxelization::createImageView(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = type;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void Voxelization::createRenderPass()
{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_8_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//Subpasses and attachment references
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = NULL;
		
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;	
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());;
		renderPassInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
}

void Voxelization::createFramebuffer()
{
	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = renderPass;
	fbufCreateInfo.pAttachments = &outputImageView;
	fbufCreateInfo.attachmentCount = 1;

	fbufCreateInfo.width = (uint32_t)(extent2D.width);
	fbufCreateInfo.height = (uint32_t)(extent2D.height);
	fbufCreateInfo.layers = LayerCount;

	if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void Voxelization::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0;// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void Voxelization::draw()
{
	
}

void Voxelization::createCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(commandBuffer, &beginInfo);


	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent2D;

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VXGIMaterials[0]->pipeline);

	for (size_t k = 0; k < standardObject->geos.size(); k++)
	{
		{
			VkBuffer vertexBuffers[] = { standardObject->geos[k]->vertexBuffer };
			VkBuffer indexBuffer = standardObject->geos[k]->indexBuffer;
			VkDeviceSize offsets[] = { 0 };				

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VXGIMaterials[k]->pipelineLayout, 0, 1, &VXGIMaterials[k]->descriptorSet, 0, nullptr);

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(standardObject->geos[k]->indices.size()), 1, 0, 0, 0);
		}
	}
	
	vkCmdEndRenderPass(commandBuffer);

	
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}




void Voxelization::createOctreeCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &octreeCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(octreeCommandBuffer, &beginInfo);

	vkCmdBindPipeline(octreeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIOctreeMaterial->computePipeline);
	vkCmdBindDescriptorSets(octreeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIOctreeMaterial->pipelineLayout, 0, 1, &VXGIOctreeMaterial->descriptorSet, 0, nullptr);
	vkCmdDispatch(octreeCommandBuffer, VXGIOctreeMaterial->computeDispatchSize.x, VXGIOctreeMaterial->computeDispatchSize.y, VXGIOctreeMaterial->computeDispatchSize.z);

	/*
	//Tag
	vkCmdBindPipeline(octreeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGITagMaterial->computePipeline);
	vkCmdBindDescriptorSets(octreeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGITagMaterial->pipelineLayout, 0, 1, &VXGITagMaterial->descriptorSet, 0, nullptr);
	vkCmdDispatch(octreeCommandBuffer, VXGITagMaterial->computeDispatchSize.x, VXGITagMaterial->computeDispatchSize.y, VXGITagMaterial->computeDispatchSize.z);
	*/
	//vkQueueWaitIdle(VXGITagMaterial->queue);
	
	if (vkEndCommandBuffer(octreeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}


void Voxelization::createAllocateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &allocateCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(allocateCommandBuffer, &beginInfo);

	//Alloaction
	vkCmdBindPipeline(allocateCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIAllocMaterial->computePipeline);
	vkCmdBindDescriptorSets(allocateCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIAllocMaterial->pipelineLayout, 0, 1, &VXGIAllocMaterial->descriptorSet, 0, nullptr);
	vkCmdDispatch(allocateCommandBuffer, VXGIAllocMaterial->computeDispatchSize.x, VXGIAllocMaterial->computeDispatchSize.y, VXGIAllocMaterial->computeDispatchSize.z);
	//Init
	//vkQueueWaitIdle(VXGIAllocMaterial->queue);

	if (vkEndCommandBuffer(allocateCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Voxelization::createMipmapCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &mipMapCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(mipMapCommandBuffer, &beginInfo);

	
	vkCmdBindPipeline(mipMapCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIMipmapMaterial->computePipeline);
	vkCmdBindDescriptorSets(mipMapCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGIMipmapMaterial->pipelineLayout, 0, 1, &VXGIMipmapMaterial->descriptorSet, 0, nullptr);
	vkCmdDispatch(mipMapCommandBuffer, VXGIMipmapMaterial->computeDispatchSize.x, VXGIMipmapMaterial->computeDispatchSize.y, VXGIMipmapMaterial->computeDispatchSize.z);
	

	if (vkEndCommandBuffer(mipMapCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Voxelization::createTextureCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &textureCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(textureCommandBuffer, &beginInfo);


	vkCmdBindPipeline(textureCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGITextureMaterial->computePipeline);
	vkCmdBindDescriptorSets(textureCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VXGITextureMaterial->pipelineLayout, 0, 1, &VXGITextureMaterial->descriptorSet, 0, nullptr);
	vkCmdDispatch(textureCommandBuffer, VXGITextureMaterial->computeDispatchSize.x, VXGITextureMaterial->computeDispatchSize.y, VXGITextureMaterial->computeDispatchSize.z);


	if (vkEndCommandBuffer(textureCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

