#include "PostProcess.h"

void PostProcess::Initialize(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkSurfaceKHR surfaceParam, VkExtent2D* extent2DParam, int LayerCountParam, uint32_t miplevelParam, glm::vec2 Scales,
	bool compute, unsigned int drawModeParam, uint32_t vertexSizeParam, bool bDepth, VkImageView depthImageParam)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	surface = surfaceParam;
	pExtent2D = extent2DParam;
	LayerCount = LayerCountParam;

	miplevel = miplevelParam;

	widthScale = 1.0f / Scales.x;
	heightScale = 1.0f / Scales.y;

	isCSPostProcess = compute;

	bActivate = true;

	drawMode = drawModeParam;

	vertexSize = vertexSizeParam;

	NeedDepth = bDepth;

	//vrMode = true;

	//depthImageView = depthImageParam;
}

void PostProcess::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;// (uint32_t)(widthScale * pExtent2D->width);
	imageInfo.extent.height = height;// (uint32_t)(heightScale * pExtent2D->height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = miplevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
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

uint32_t PostProcess::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

void PostProcess::createImages()
{
	if (vrMode)
	{
		createImage((uint32_t)(widthScale * pExtent2D->width * 0.5f), (uint32_t)(heightScale * pExtent2D->height), format, tiling, usage, properties, outputImage, outputImageMemory);
		outputImageView = createImageView(outputImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	else
	{
		createImage((uint32_t)(widthScale * pExtent2D->width), (uint32_t)(heightScale * pExtent2D->height), format, tiling, usage, properties, outputImage, outputImageMemory);
		outputImageView = createImageView(outputImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	
}


void PostProcess::createImages(VkFormat formatParam, VkImageTiling tilingParam, VkImageUsageFlags usageParam, VkMemoryPropertyFlags propertiesParam)
{
	format = formatParam;
	tiling = tilingParam;
	usage = usageParam;
	properties = propertiesParam;


	createImage((uint32_t)(widthScale * pExtent2D->width), (uint32_t)(heightScale * pExtent2D->height), format, tiling, usage, properties, outputImage, outputImageMemory);
	outputImageView = createImageView(outputImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView PostProcess::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
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

void PostProcess::createRenderPass()
{
	if(!isCSPostProcess)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
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


		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // We will read from depth, so it's important to store the depth attachment results
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // Attachment will be transitioned to shader read at render pass end

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		
		
		if(NeedDepth)
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
		else
			subpass.pDepthStencilAttachment = NULL;
		


		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		
		if(NeedDepth)
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		else
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		

		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		
		if (NeedDepth)
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		else
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		

		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		
		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		if (NeedDepth)
		{
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();
		}
		else
		{
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());;
			renderPassInfo.pDependencies = dependencies.data();
		}	
		
		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	
}

void PostProcess::createFramebuffer()
{
	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = renderPass;	

	std::array<VkImageView, 2> attachments = { outputImageView, depthImageView };

	
	if (NeedDepth)
	{		
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbufCreateInfo.pAttachments = attachments.data();
		
	}
	else
	{		
		fbufCreateInfo.attachmentCount = 1;
		fbufCreateInfo.pAttachments = &outputImageView;
	}
	

	if (vrMode)
	{
		fbufCreateInfo.width = (uint32_t)(widthScale * pExtent2D->width * 0.5f);
	}
	else
	{
		fbufCreateInfo.width = (uint32_t)(widthScale * pExtent2D->width);
	}

	
	fbufCreateInfo.height = (uint32_t)(heightScale * pExtent2D->height);
	fbufCreateInfo.layers = LayerCount;

	if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void PostProcess::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void PostProcess::createCommandBuffers()
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
	

	if (material->pipeline)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };

		VkExtent2D extent2D;

		if (vrMode)
		{
			extent2D.width = (uint32_t)(widthScale * pExtent2D->width * 0.5f);
		}
		else
		{
			extent2D.width = (uint32_t)(widthScale * pExtent2D->width);
		}

		extent2D.height = (uint32_t)(heightScale * pExtent2D->height);
		renderPassInfo.renderArea.extent = extent2D;
	

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		
		if (NeedDepth)
		{
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
		}
		else
		{					
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearValues[0];
		}

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipelineLayout, 0, 1, &material->descriptorSet, 0, nullptr);

		VkDeviceSize offsets[] = { 0 };

		if (drawMode == 0)
		{
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &material->vertexBuffer, offsets);
			vkCmdDraw(commandBuffer, vertexSize, 1, 0, 0);
		
		}
		else if (drawMode == 1)
		{	

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &offScreenPlane->vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, offScreenPlane->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(offScreenPlane->indices.size()), 1, 0, 0, 0);			
		}

		
		vkCmdEndRenderPass(commandBuffer);

	}
	else if (material->computePipeline)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, material->computePipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, material->pipelineLayout, 0, 1, &material->descriptorSet, 0, nullptr);
		vkCmdDispatch(commandBuffer, material->computeDispatchSize.x, material->computeDispatchSize.y, material->computeDispatchSize.z);
	}

	

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}