#include "Material.h"

Material::Material():widthScale(1.0f), heightScale(1.0f)
{
	pipeline = NULL;
	pipeline2 = NULL;
	computePipeline = NULL;

	vrMode = false;

	renderPass = NULL;
	pipelineLayout = NULL;
}

Material::~Material()
{
	cleanUp();
}

void Material::createUniformBuffer()
{
	createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}


void Material::setShaderPaths(std::string v, std::string f, std::string c, std::string e, std::string g, std::string p)
{
	vertexShaderPath = v;
	fragmentShaderPath = f;
	tessellationControlShaderPath = c;
	tessellationEvaluationShaderPath = e;
	geometryShaderPath = g;
	computeShaderPath = p;
}

void Material::setScreenScale(glm::vec2 scale)
{
	widthScale = 1.0f / scale.x;
	heightScale = 1.0f / scale.y;
}

void Material::addTexture(Texture* texture)
{
	textures.push_back(texture);
}

void Material::LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	commandPool = commandPoolParam;
	queue = queueParam;

	path = pathParam;

	createDescriptorSetLayout();
	createUniformBuffer();
	createDescriptorPool();
}



VkShaderModule Material::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void Material::cleanPipeline()
{
	if (fragmentShaderPath != "")
	{
		if(pipeline != NULL)
		vkDestroyPipeline(device, pipeline, nullptr);

		if (pipeline2 != NULL)
		vkDestroyPipeline(device, pipeline2, nullptr);
	}	
	if(computeShaderPath != "")
		vkDestroyPipeline(device, computePipeline, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void Material::cleanUp()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);

	vkDestroyBuffer(device, directionalLightBuffer, nullptr);
	vkFreeMemory(device, directionalLightBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void ObjectDrawMaterial::createGraphicsPipeline(VkExtent2D swapChainExtent)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//extent = swapChainExtent;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	
	if (vrMode)
	{
		viewport.width = static_cast<float>(swapChainExtent.width) * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(swapChainExtent.width);
	}
	
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };

	
	if (vrMode)
	{
		VkExtent2D vrExtent = swapChainExtent;
		vrExtent.width /= 2;
		scissor.extent = vrExtent;
	}
	else
	{
		scissor.extent = swapChainExtent;
	}

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode =  VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendAttachmentState specColorBlendAttachment = {};
	specColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	specColorBlendAttachment.blendEnable = VK_FALSE;
	specColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	specColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	specColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	specColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	specColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	specColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendAttachmentState normalColorBlendAttachment = {};
	normalColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	normalColorBlendAttachment.blendEnable = VK_FALSE;
	normalColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	normalColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	normalColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	normalColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	normalColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	normalColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendAttachmentState emissiveColorBlendAttachment = {};
	emissiveColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	emissiveColorBlendAttachment.blendEnable = VK_FALSE;
	emissiveColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	emissiveColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	emissiveColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	emissiveColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	emissiveColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	emissiveColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, NUM_GBUFFERS> gBuffersAttachment = { colorBlendAttachment, specColorBlendAttachment, normalColorBlendAttachment, emissiveColorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(gBuffersAttachment.size());
	colorBlending.pAttachments = gBuffersAttachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}


	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_TRUE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void ObjectDrawMaterial::createDescriptorSetLayout()
{
	//BasicColor
	VkDescriptorSetLayoutBinding basicColorSamplerLayoutBinding = {};
	basicColorSamplerLayoutBinding.binding = 0;
	basicColorSamplerLayoutBinding.descriptorCount = 1;
	basicColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	basicColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	basicColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//SpecColor
	VkDescriptorSetLayoutBinding specularColorSamplerLayoutBinding = {};
	specularColorSamplerLayoutBinding.binding = 1;
	specularColorSamplerLayoutBinding.descriptorCount = 1;
	specularColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	specularColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	specularColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//NormalColor
	VkDescriptorSetLayoutBinding normalColorSamplerLayoutBinding = {};
	normalColorSamplerLayoutBinding.binding = 2;
	normalColorSamplerLayoutBinding.descriptorCount = 1;
	normalColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	normalColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//EmissiveColor
	VkDescriptorSetLayoutBinding emissiveColorSamplerLayoutBinding = {};
	emissiveColorSamplerLayoutBinding.binding = 3;
	emissiveColorSamplerLayoutBinding.descriptorCount = 1;
	emissiveColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	emissiveColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	emissiveColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 4;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 5;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { basicColorSamplerLayoutBinding, specularColorSamplerLayoutBinding, normalColorSamplerLayoutBinding, emissiveColorSamplerLayoutBinding, uboLayoutBinding, fuboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void ObjectDrawMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 6> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[5].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	
	//??
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1; //1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void ObjectDrawMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	basicColorImageInfo.imageView = textures[BASIC_COLOR]->textureImageView;
	basicColorImageInfo.sampler = textures[BASIC_COLOR]->textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	specularColorImageInfo.imageView = textures[SPECULAR_COLOR]->textureImageView;
	specularColorImageInfo.sampler = textures[SPECULAR_COLOR]->textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalColorImageInfo.imageView = textures[NORMAL_COLOR]->textureImageView;
	normalColorImageInfo.sampler = textures[NORMAL_COLOR]->textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	emissiveColorImageInfo.imageView = textures[EMISSIVE_COLOR]->textureImageView;
	emissiveColorImageInfo.sampler = textures[EMISSIVE_COLOR]->textureSampler;

	std::array<VkWriteDescriptorSet, 6> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VoxelizeMaterial::createGraphicsPipeline(VkExtent2D swapChainExtent)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto geoShaderCode = readFile(geometryShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule geoShaderModule = createShaderModule(geoShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geoShaderStageInfo = {};
	geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geoShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geoShaderStageInfo.module = geoShaderModule;
	geoShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo, geoShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//extent = swapChainExtent;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };


	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = {colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}



	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 3;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, geoShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void VoxelizeMaterial::createDescriptorSetLayout()
{	
	//BasicColor
	VkDescriptorSetLayoutBinding basicColorSamplerLayoutBinding = {};
	basicColorSamplerLayoutBinding.binding = 0;
	basicColorSamplerLayoutBinding.descriptorCount = 1;
	basicColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	basicColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	basicColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	//SpecColor
	VkDescriptorSetLayoutBinding specularColorSamplerLayoutBinding = {};
	specularColorSamplerLayoutBinding.binding = 1;
	specularColorSamplerLayoutBinding.descriptorCount = 1;
	specularColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	specularColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	specularColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//NormalColor
	VkDescriptorSetLayoutBinding normalColorSamplerLayoutBinding = {};
	normalColorSamplerLayoutBinding.binding = 2;
	normalColorSamplerLayoutBinding.descriptorCount = 1;
	normalColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	normalColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//EmissiveColor
	VkDescriptorSetLayoutBinding emissiveColorSamplerLayoutBinding = {};
	emissiveColorSamplerLayoutBinding.binding = 3;
	emissiveColorSamplerLayoutBinding.descriptorCount = 1;
	emissiveColorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	emissiveColorSamplerLayoutBinding.pImmutableSamplers = nullptr;
	emissiveColorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 4;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 5;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding voxeluboLayoutBinding = {};
	voxeluboLayoutBinding.binding = 6;
	voxeluboLayoutBinding.descriptorCount = 1;
	voxeluboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	voxeluboLayoutBinding.pImmutableSamplers = nullptr;
	voxeluboLayoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3DTextureLayoutBinding = {};
	albedo3DTextureLayoutBinding.binding = 7;
	albedo3DTextureLayoutBinding.descriptorCount = 1;
	albedo3DTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	albedo3DTextureLayoutBinding.pImmutableSamplers = nullptr;
	albedo3DTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 8> bindings = { basicColorSamplerLayoutBinding, specularColorSamplerLayoutBinding, normalColorSamplerLayoutBinding, emissiveColorSamplerLayoutBinding,
		uboLayoutBinding, fuboLayoutBinding, voxeluboLayoutBinding, /*voxelFragListLayoutBinding, outputAlbedoTextureLayoutBinding, outputPosTextureLayoutBinding,*/ albedo3DTextureLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelizeMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 8> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[5].descriptorCount = 1;

	poolSizes[6].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[6].descriptorCount = 1;

	/*
	poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[7].descriptorCount = 1;

	poolSizes[8].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[8].descriptorCount = 1;

	poolSizes[9].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[9].descriptorCount = 1;
	*/

	poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[7].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	//??
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1; //1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelizeMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkSampler texture3DSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &texture3DSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorBufferInfo voxelbufferInfo = {};
	voxelbufferInfo.buffer = voxelUniformBuffer;
	voxelbufferInfo.offset = 0;
	voxelbufferInfo.range = sizeof(VoxelUniformBufferObject);

	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	basicColorImageInfo.imageView = textures[BASIC_COLOR]->textureImageView;
	basicColorImageInfo.sampler = textures[BASIC_COLOR]->textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularColorImageInfo.imageView = textures[SPECULAR_COLOR]->textureImageView;
	specularColorImageInfo.sampler = textures[SPECULAR_COLOR]->textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalColorImageInfo.imageView = textures[NORMAL_COLOR]->textureImageView;
	normalColorImageInfo.sampler = textures[NORMAL_COLOR]->textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveColorImageInfo.imageView = textures[EMISSIVE_COLOR]->textureImageView;
	emissiveColorImageInfo.sampler = textures[EMISSIVE_COLOR]->textureSampler;

	/*
	VkDescriptorBufferInfo voxelFragCountbufferInfo = {};
	voxelFragCountbufferInfo.buffer = voxelFragCountBuffer;
	voxelFragCountbufferInfo.offset = 0;
	voxelFragCountbufferInfo.range = sizeof(VoxelFragCount);

	VkDescriptorBufferInfo outputPosbufferInfo = {};
	outputPosbufferInfo.buffer = ouputPosListBuffer;
	outputPosbufferInfo.offset = 0;
	outputPosbufferInfo.range = sizeof(FragmentListData) * fragCount;

	VkDescriptorBufferInfo outputAlbedobufferInfo = {};
	outputAlbedobufferInfo.buffer = ouputAlbedoListBuffer;
	outputAlbedobufferInfo.offset = 0;
	outputAlbedobufferInfo.range = sizeof(FragmentListData) * fragCount;
	*/

	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	albedo3DImageInfo.imageView = albedo3DImageView;
	albedo3DImageInfo.sampler = texture3DSampler;

	std::array<VkWriteDescriptorSet, 8> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;
	
	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pBufferInfo = &voxelbufferInfo;

	/*
	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pBufferInfo = &voxelFragCountbufferInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pBufferInfo = &outputPosbufferInfo;

	descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[9].dstSet = descriptorSet;
	descriptorWrites[9].dstBinding = 9;
	descriptorWrites[9].dstArrayElement = 0;
	descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[9].descriptorCount = 1;
	descriptorWrites[9].pBufferInfo = &outputAlbedobufferInfo;
	*/

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &albedo3DImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, texture3DSampler, nullptr);
}

void VoxelTagMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding LB00 = {};
	LB00.binding = 0;
	LB00.descriptorCount = 1;
	LB00.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB00.pImmutableSamplers = nullptr;
	LB00.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB01 = {};
	LB01.binding = 1;
	LB01.descriptorCount = 1;
	LB01.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB01.pImmutableSamplers = nullptr;
	LB01.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB02 = {};
	LB02.binding = 2;
	LB02.descriptorCount = 1;
	LB02.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB02.pImmutableSamplers = nullptr;
	LB02.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB03 = {};
	LB03.binding = 3;
	LB03.descriptorCount = 1;
	LB03.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LB03.pImmutableSamplers = nullptr;
	LB03.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB04 = {};
	LB04.binding = 4;
	LB04.descriptorCount = 1;
	LB04.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB04.pImmutableSamplers = nullptr;
	LB04.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB05 = {};
	LB05.binding = 5;
	LB05.descriptorCount = 1;
	LB05.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB05.pImmutableSamplers = nullptr;
	LB05.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { LB00, LB01, LB02, LB03, LB04, LB05 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelTagMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 6> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[5].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelTagMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}	

	VkDescriptorBufferInfo voxelFragCountbufferInfo = {};
	voxelFragCountbufferInfo.buffer = voxelFragCountBuffer;
	voxelFragCountbufferInfo.offset = 0;
	voxelFragCountbufferInfo.range = sizeof(VoxelFragCount);


	VkDescriptorBufferInfo OctreeInfo = {};
	OctreeInfo.buffer = voxelOctreeBuffer;
	OctreeInfo.offset = 0;
	OctreeInfo.range = sizeof(SVONode) * maxNode;

	VkDescriptorBufferInfo posInfo = {};
	posInfo.buffer = voxelPosBuffer;
	posInfo.offset = 0;
	posInfo.range = sizeof(FragmentListData) * fragCount;

	VkDescriptorBufferInfo SVOInfo = {};
	SVOInfo.buffer = SVOInitInfoBuffer;
	SVOInfo.offset = 0;
	SVOInfo.range = sizeof(SVOInitInfo);

	VkDescriptorBufferInfo NumChildInfo = {};
	NumChildInfo.buffer = NumChildBuffer;
	NumChildInfo.offset = 0;
	NumChildInfo.range = sizeof(SVOTagOutput);

	VkDescriptorBufferInfo albedoInfo = {};
	albedoInfo.buffer = voxelAlbedoBuffer;
	albedoInfo.offset = 0;
	albedoInfo.range = sizeof(FragmentListData) * fragCount;

	std::array<VkWriteDescriptorSet, 6> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &voxelFragCountbufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &OctreeInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &posInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &SVOInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &NumChildInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &albedoInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VoxelTagMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);

	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}


void VoxelAllocationMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding LB00 = {};
	LB00.binding = 0;
	LB00.descriptorCount = 1;
	LB00.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB00.pImmutableSamplers = nullptr;
	LB00.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB01 = {};
	LB01.binding = 1;
	LB01.descriptorCount = 1;
	LB01.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB01.pImmutableSamplers = nullptr;
	LB01.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB02 = {};
	LB02.binding = 2;
	LB02.descriptorCount = 1;
	LB02.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB02.pImmutableSamplers = nullptr;
	LB02.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB03 = {};
	LB03.binding = 3;
	LB03.descriptorCount = 1;
	LB03.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LB03.pImmutableSamplers = nullptr;
	LB03.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB04 = {};
	LB04.binding = 4;
	LB04.descriptorCount = 1;
	LB04.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB04.pImmutableSamplers = nullptr;
	LB04.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 5> bindings = { LB00, LB01, LB02, LB03, LB04 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelAllocationMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 5> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[4].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelAllocationMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo voxelFragCountbufferInfo = {};
	voxelFragCountbufferInfo.buffer = voxelFragCountBuffer;
	voxelFragCountbufferInfo.offset = 0;
	voxelFragCountbufferInfo.range = sizeof(VoxelFragCount);


	VkDescriptorBufferInfo OctreeInfo = {};
	OctreeInfo.buffer = voxelOctreeBuffer;
	OctreeInfo.offset = 0;
	OctreeInfo.range = sizeof(SVONode) * maxNode;

	VkDescriptorBufferInfo posInfo = {};
	posInfo.buffer = voxelPosBuffer;
	posInfo.offset = 0;
	posInfo.range = sizeof(FragmentListData) * fragCount;

	VkDescriptorBufferInfo SVOInfo = {};
	SVOInfo.buffer = SVOInitInfoBuffer;
	SVOInfo.offset = 0;
	SVOInfo.range = sizeof(SVOInitInfo);

	VkDescriptorBufferInfo NumChildInfo = {};
	NumChildInfo.buffer = NumChildBuffer;
	NumChildInfo.offset = 0;
	NumChildInfo.range = sizeof(SVOTagOutput);

	std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &voxelFragCountbufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &OctreeInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &posInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &SVOInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &NumChildInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VoxelAllocationMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);

	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}


void VoxelMipmapMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding LB00 = {};
	LB00.binding = 0;
	LB00.descriptorCount = 1;
	LB00.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB00.pImmutableSamplers = nullptr;
	LB00.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB01 = {};
	LB01.binding = 1;
	LB01.descriptorCount = 1;
	LB01.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB01.pImmutableSamplers = nullptr;
	LB01.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB02 = {};
	LB02.binding = 2;
	LB02.descriptorCount = 1;
	LB02.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB02.pImmutableSamplers = nullptr;
	LB02.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB03 = {};
	LB03.binding = 3;
	LB03.descriptorCount = 1;
	LB03.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LB03.pImmutableSamplers = nullptr;
	LB03.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB04 = {};
	LB04.binding = 4;
	LB04.descriptorCount = 1;
	LB04.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB04.pImmutableSamplers = nullptr;
	LB04.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB05 = {};
	LB05.binding = 5;
	LB05.descriptorCount = 1;
	LB05.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB05.pImmutableSamplers = nullptr;
	LB05.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { LB00, LB01, LB02, LB03, LB04, LB05 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelMipmapMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 6> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[5].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelMipmapMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo voxelFragCountbufferInfo = {};
	voxelFragCountbufferInfo.buffer = voxelFragCountBuffer;
	voxelFragCountbufferInfo.offset = 0;
	voxelFragCountbufferInfo.range = sizeof(VoxelFragCount);


	VkDescriptorBufferInfo OctreeInfo = {};
	OctreeInfo.buffer = voxelOctreeBuffer;
	OctreeInfo.offset = 0;
	OctreeInfo.range = sizeof(SVONode) * maxNode;

	VkDescriptorBufferInfo posInfo = {};
	posInfo.buffer = voxelPosBuffer;
	posInfo.offset = 0;
	posInfo.range = sizeof(FragmentListData) * fragCount;

	VkDescriptorBufferInfo SVOInfo = {};
	SVOInfo.buffer = SVOInitInfoBuffer;
	SVOInfo.offset = 0;
	SVOInfo.range = sizeof(SVOInitInfo);

	VkDescriptorBufferInfo NumChildInfo = {};
	NumChildInfo.buffer = NumChildBuffer;
	NumChildInfo.offset = 0;
	NumChildInfo.range = sizeof(SVOTagOutput);

	VkDescriptorBufferInfo albedoInfo = {};
	albedoInfo.buffer = voxelAlbedoBuffer;
	albedoInfo.offset = 0;
	albedoInfo.range = sizeof(FragmentListData) * fragCount;

	std::array<VkWriteDescriptorSet, 6> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &voxelFragCountbufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &OctreeInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &posInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &SVOInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &NumChildInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &albedoInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VoxelMipmapMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);

	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}



void VoxelTextureMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding LB00 = {};
	LB00.binding = 0;
	LB00.descriptorCount = 1;
	LB00.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LB00.pImmutableSamplers = nullptr;
	LB00.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB01 = {};
	LB01.binding = 1;
	LB01.descriptorCount = 1;
	LB01.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	LB01.pImmutableSamplers = nullptr;
	LB01.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB02 = {};
	LB02.binding = 2;
	LB02.descriptorCount = 1;
	LB02.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	LB02.pImmutableSamplers = nullptr;
	LB02.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { LB00, LB01, LB02};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelTextureMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelTextureMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}


	VkSampler texture3DSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &texture3DSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	
	VkDescriptorBufferInfo SVOInfo = {};
	SVOInfo.buffer = SVOInitInfoBuffer;
	SVOInfo.offset = 0;
	SVOInfo.range = sizeof(SVOInitInfo);
	
	VkDescriptorImageInfo upperalbedo3DImageInfo = {};
	upperalbedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	upperalbedo3DImageInfo.imageView = upperalbedo3DImageView;
	upperalbedo3DImageInfo.sampler = texture3DSampler;

	VkDescriptorImageInfo loweralbedo3DImageInfo = {};
	loweralbedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	loweralbedo3DImageInfo.imageView = loweralbedo3DImageView;
	loweralbedo3DImageInfo.sampler = texture3DSampler;
	

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &SVOInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &upperalbedo3DImageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &loweralbedo3DImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, texture3DSampler, nullptr);
}

void VoxelTextureMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);

	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}


void VoxelOctreeMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding LB00 = {};
	LB00.binding = 0;
	LB00.descriptorCount = 1;
	LB00.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LB00.pImmutableSamplers = nullptr;
	LB00.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB01 = {};
	LB01.binding = 1;
	LB01.descriptorCount = 1;
	LB01.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB01.pImmutableSamplers = nullptr;
	LB01.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB02 = {};
	LB02.binding = 2;
	LB02.descriptorCount = 1;
	LB02.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB02.pImmutableSamplers = nullptr;
	LB02.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding LB03 = {};
	LB03.binding = 3;
	LB03.descriptorCount = 1;
	LB03.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	LB03.pImmutableSamplers = nullptr;
	LB03.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> bindings = { LB00, LB01, LB02, LB03};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelOctreeMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[3].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelOctreeMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}


	VkSampler texture3DSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &texture3DSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorBufferInfo SVOInfo = {};
	SVOInfo.buffer = SVOInitInfoBuffer;
	SVOInfo.offset = 0;
	SVOInfo.range = sizeof(SVOInitInfo);

	VkDescriptorBufferInfo OctreeBufferInfo = {};
	OctreeBufferInfo.buffer = OCtreeBuffer;
	OctreeBufferInfo.offset = 0;
	OctreeBufferInfo.range = sizeof(OctreeNode) * numOCtreeNode;

	VkDescriptorBufferInfo MemIndicBufferInfo = {};
	MemIndicBufferInfo.buffer = MemIndcBuffer;
	MemIndicBufferInfo.offset = 0;
	MemIndicBufferInfo.range = sizeof(MemoryIndicator);


	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	albedo3DImageInfo.imageView = albedo3DImageView;
	albedo3DImageInfo.sampler = texture3DSampler;


	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &SVOInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &OctreeBufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &MemIndicBufferInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &albedo3DImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, texture3DSampler, nullptr);
}

void VoxelOctreeMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);

	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}


void VoxelRenderMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 1;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding voxelInfoBinding = {};
	voxelInfoBinding.binding = 2;
	voxelInfoBinding.descriptorCount = 1;
	voxelInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	voxelInfoBinding.pImmutableSamplers = nullptr;
	voxelInfoBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding outputPosTextureLayoutBinding = {};
	outputPosTextureLayoutBinding.binding = 3;
	outputPosTextureLayoutBinding.descriptorCount = 1;
	outputPosTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	outputPosTextureLayoutBinding.pImmutableSamplers = nullptr;
	outputPosTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding outputAlbedoTextureLayoutBinding = {};
	outputAlbedoTextureLayoutBinding.binding = 4;
	outputAlbedoTextureLayoutBinding.descriptorCount = 1;
	outputAlbedoTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	outputAlbedoTextureLayoutBinding.pImmutableSamplers = nullptr;
	outputAlbedoTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VkDescriptorSetLayoutBinding LB03 = {};
	LB03.binding = 5;
	LB03.descriptorCount = 1;
	LB03.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	LB03.pImmutableSamplers = nullptr;
	LB03.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding LB06 = {};
	LB06.binding = 6;
	LB06.descriptorCount = 1;
	LB06.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	LB06.pImmutableSamplers = nullptr;
	LB06.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 7> bindings = { uboLayoutBinding, fuboLayoutBinding, voxelInfoBinding, outputPosTextureLayoutBinding, outputAlbedoTextureLayoutBinding, LB03,
		LB06 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelRenderMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 7> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[5].descriptorCount = 1;

	poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[6].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelRenderMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler texture3DSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &texture3DSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorBufferInfo voxelInfo = {};
	voxelInfo.buffer = voxelInfoBuffer;
	voxelInfo.offset = 0;
	voxelInfo.range = sizeof(VoxelInfo);


	VkDescriptorBufferInfo outputPosbufferInfo = {};
	outputPosbufferInfo.buffer = ouputPosListBuffer;
	outputPosbufferInfo.offset = 0;
	outputPosbufferInfo.range = sizeof(FragmentListData) * fragListVoxelCount;

	VkDescriptorBufferInfo outputAlbedobufferInfo = {};
	outputAlbedobufferInfo.buffer = ouputAlbedoListBuffer;
	outputAlbedobufferInfo.offset = 0;
	outputAlbedobufferInfo.range = sizeof(FragmentListData) * fragListVoxelCount;


// 	VkDescriptorBufferInfo OctreeInfo = {};
// 	OctreeInfo.buffer = voxelOctreeBuffer;
// 	OctreeInfo.offset = 0;
// 	OctreeInfo.range = sizeof(SVONode) * maxNode;

	VkDescriptorBufferInfo OctreeInfo = {};
	OctreeInfo.buffer = ouputAlbedoListBuffer;
	OctreeInfo.offset = 0;
	OctreeInfo.range = sizeof(OctreeNode) * maxNode;

	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	albedo3DImageInfo.imageView = albedo3DImageView;
	albedo3DImageInfo.sampler = texture3DSampler;

	std::array<VkWriteDescriptorSet, 7> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &fragbufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &voxelInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &outputPosbufferInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &outputAlbedobufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &OctreeInfo;


	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &albedo3DImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, texture3DSampler, nullptr);
}

void VoxelRenderMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler texture3DSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &texture3DSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorBufferInfo voxelInfo = {};
	voxelInfo.buffer = voxelInfoBuffer;
	voxelInfo.offset = 0;
	voxelInfo.range = sizeof(VoxelInfo);

	VkDescriptorBufferInfo outputPosbufferInfo = {};
	outputPosbufferInfo.buffer = ouputPosListBuffer;
	outputPosbufferInfo.offset = 0;
	outputPosbufferInfo.range = sizeof(FragmentListData) * fragListVoxelCount;

	VkDescriptorBufferInfo outputAlbedobufferInfo = {};
	outputAlbedobufferInfo.buffer = ouputAlbedoListBuffer;
	outputAlbedobufferInfo.offset = 0;
	outputAlbedobufferInfo.range = sizeof(FragmentListData) * fragListVoxelCount;

	// 	VkDescriptorBufferInfo OctreeInfo = {};
	// 	OctreeInfo.buffer = voxelOctreeBuffer;
	// 	OctreeInfo.offset = 0;
	// 	OctreeInfo.range = sizeof(SVONode) * maxNode;

	VkDescriptorBufferInfo OctreeInfo = {};
	OctreeInfo.buffer = ouputAlbedoListBuffer;
	OctreeInfo.offset = 0;
	OctreeInfo.range = sizeof(OctreeNode) * maxNode;

	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	albedo3DImageInfo.imageView = albedo3DImageView;
	albedo3DImageInfo.sampler = texture3DSampler;

	std::array<VkWriteDescriptorSet, 7> descriptorWrites = {};
	
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &fragbufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &voxelInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &outputPosbufferInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &outputAlbedobufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &OctreeInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &albedo3DImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, texture3DSampler, nullptr);
}

void VoxelRenderMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto geoShaderCode = readFile(geometryShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule geoShaderModule = createShaderModule(geoShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geoShaderStageInfo = {};
	geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geoShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geoShaderStageInfo.module = geoShaderModule;
	geoShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo, geoShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;


	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}


	viewport.height = Extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}


	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_TRUE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 3;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, geoShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void LightingMaterial::createDescriptorSetLayout()
{
	//BasicColorRenderTarget
	VkDescriptorSetLayoutBinding BasicColorRenderTargetLayoutBinding = {};
	BasicColorRenderTargetLayoutBinding.binding = 0;
	BasicColorRenderTargetLayoutBinding.descriptorCount = 1;
	BasicColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	BasicColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	BasicColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//SpecColorRenderTarget
	VkDescriptorSetLayoutBinding SpecColorRenderTargetLayoutBinding = {};
	SpecColorRenderTargetLayoutBinding.binding = 1;
	SpecColorRenderTargetLayoutBinding.descriptorCount = 1;
	SpecColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SpecColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SpecColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//NormalColorRenderTarget
	VkDescriptorSetLayoutBinding NormalColorRenderTargetLayoutBinding = {};
	NormalColorRenderTargetLayoutBinding.binding = 2;
	NormalColorRenderTargetLayoutBinding.descriptorCount = 1;
	NormalColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	NormalColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	NormalColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//EmissiveColorRenderTarget
	VkDescriptorSetLayoutBinding EmissiveColorRenderTargetLayoutBinding = {};
	EmissiveColorRenderTargetLayoutBinding.binding = 3;
	EmissiveColorRenderTargetLayoutBinding.descriptorCount = 1;
	EmissiveColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	EmissiveColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	EmissiveColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 4;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 5;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding DepthTextureLayoutBinding = {};
	DepthTextureLayoutBinding.binding = 6;
	DepthTextureLayoutBinding.descriptorCount = 1;
	DepthTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DepthTextureLayoutBinding.pImmutableSamplers = nullptr;
	DepthTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding directionalLayoutBinding = {};
	directionalLayoutBinding.binding = 7;
	directionalLayoutBinding.descriptorCount = 1;
	directionalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	directionalLayoutBinding.pImmutableSamplers = nullptr;
	directionalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//shadow
	VkDescriptorSetLayoutBinding ShadowMapLB = {};
	ShadowMapLB.binding = 8;
	ShadowMapLB.descriptorCount = 1;
	ShadowMapLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ShadowMapLB.pImmutableSamplers = nullptr;
	ShadowMapLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding shadowUBOLB = {};
	shadowUBOLB.binding = 9;
	shadowUBOLB.descriptorCount = 1;
	shadowUBOLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadowUBOLB.pImmutableSamplers = nullptr;
	shadowUBOLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding GIMapLB = {};
	GIMapLB.binding = 10;
	GIMapLB.descriptorCount = 1;
	GIMapLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	GIMapLB.pImmutableSamplers = nullptr;
	GIMapLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding optionLB = {};
	optionLB.binding = 11;
	optionLB.descriptorCount = 1;
	optionLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	optionLB.pImmutableSamplers = nullptr;
	optionLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 12> bindings = { BasicColorRenderTargetLayoutBinding, SpecColorRenderTargetLayoutBinding, NormalColorRenderTargetLayoutBinding, EmissiveColorRenderTargetLayoutBinding,
		uboLayoutBinding, fuboLayoutBinding, DepthTextureLayoutBinding, directionalLayoutBinding, ShadowMapLB, shadowUBOLB, GIMapLB, optionLB };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void LightingMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 12> poolSizes = {};
	poolSizes[BASIC_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[BASIC_COLOR].descriptorCount = 1;

	poolSizes[SPECULAR_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[SPECULAR_COLOR].descriptorCount = 1;

	poolSizes[NORMAL_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[NORMAL_COLOR].descriptorCount = 1;

	poolSizes[EMISSIVE_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[EMISSIVE_COLOR].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[5].descriptorCount = 1;

	poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[6].descriptorCount = 1;

	poolSizes[7].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[7].descriptorCount = 1;

	poolSizes[8].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[8].descriptorCount = 1;

	poolSizes[9].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[9].descriptorCount = 1;
	
	poolSizes[10].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[10].descriptorCount = 1;

	poolSizes[11].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[11].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size())-1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void LightingMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	

	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	basicColorImageInfo.imageView = (*gBufferImageViews)[BASIC_COLOR];
	basicColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularColorImageInfo.imageView = (*gBufferImageViews)[SPECULAR_COLOR];
	specularColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalColorImageInfo.imageView = (*gBufferImageViews)[NORMAL_COLOR];
	normalColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveColorImageInfo.imageView = (*gBufferImageViews)[EMISSIVE_COLOR];
	emissiveColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo dethImageInfo = {};
	dethImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dethImageInfo.imageView = DepthImageView;
	dethImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo directionalLightInfo = {};
	directionalLightInfo.buffer = directionalLightBuffer;
	directionalLightInfo.offset = 0;
	directionalLightInfo.range = sizeof(DirectionalLight) * (*pDirectionLights).size();

	//shadow
	VkDescriptorImageInfo shadowMapImageInfo = {};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapView;
	shadowMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo shadowUBOInfo = {};
	shadowUBOInfo.buffer = shadowConstantBuffer;
	shadowUBOInfo.offset = 0;
	shadowUBOInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorImageInfo GIMapImageInfo = {};
	GIMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	GIMapImageInfo.imageView = GIView;
	GIMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo optionInfo = {};
	optionInfo.buffer = optionBuffer;
	optionInfo.offset = 0;
	optionInfo.range = sizeof(uint32_t);

	std::array<VkWriteDescriptorSet, 12> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &dethImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pBufferInfo = &directionalLightInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &shadowMapImageInfo;

	descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[9].dstSet = descriptorSet;
	descriptorWrites[9].dstBinding = 9;
	descriptorWrites[9].dstArrayElement = 0;
	descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[9].descriptorCount = 1;
	descriptorWrites[9].pBufferInfo = &shadowUBOInfo;

	descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[10].dstSet = descriptorSet;
	descriptorWrites[10].dstBinding = 10;
	descriptorWrites[10].dstArrayElement = 0;
	descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[10].descriptorCount = 1;
	descriptorWrites[10].pImageInfo = &GIMapImageInfo;

	descriptorWrites[11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[11].dstSet = descriptorSet;
	descriptorWrites[11].dstBinding = 11;
	descriptorWrites[11].dstArrayElement = 0;
	descriptorWrites[11].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[11].descriptorCount = 1;
	descriptorWrites[11].pBufferInfo = &optionInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void LightingMaterial::createGraphicsPipeline(VkExtent2D swapChainExtent)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//extent = swapChainExtent;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	
	if (vrMode)
	{
		viewport.width = static_cast<float>(swapChainExtent.width) * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(swapChainExtent.width);
	}
	
	
	//viewport.width = static_cast<float>()swapChainExtent.width;
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };

	
	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;
	//pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void LightingMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	basicColorImageInfo.imageView = (*gBufferImageViews)[BASIC_COLOR];
	basicColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularColorImageInfo.imageView = (*gBufferImageViews)[SPECULAR_COLOR];
	specularColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalColorImageInfo.imageView = (*gBufferImageViews)[NORMAL_COLOR];
	normalColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveColorImageInfo.imageView = (*gBufferImageViews)[EMISSIVE_COLOR];
	emissiveColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo dethImageInfo = {};
	dethImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dethImageInfo.imageView = DepthImageView;
	dethImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo directionalLightInfo = {};
	directionalLightInfo.buffer = directionalLightBuffer;
	directionalLightInfo.offset = 0;
	directionalLightInfo.range = sizeof(DirectionalLight) * (*pDirectionLights).size();

	//shadow
	VkDescriptorImageInfo shadowMapImageInfo = {};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapView;
	shadowMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo shadowUBOInfo = {};
	shadowUBOInfo.buffer = shadowConstantBuffer;
	shadowUBOInfo.offset = 0;
	shadowUBOInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorImageInfo GIMapImageInfo = {};
	GIMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	GIMapImageInfo.imageView = GIView;
	GIMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo optionInfo = {};
	optionInfo.buffer = optionBuffer;
	optionInfo.offset = 0;
	optionInfo.range = sizeof(uint32_t);

	std::array<VkWriteDescriptorSet, 12> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &dethImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pBufferInfo = &directionalLightInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &shadowMapImageInfo;

	descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[9].dstSet = descriptorSet;
	descriptorWrites[9].dstBinding = 9;
	descriptorWrites[9].dstArrayElement = 0;
	descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[9].descriptorCount = 1;
	descriptorWrites[9].pBufferInfo = &shadowUBOInfo;

	descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[10].dstSet = descriptorSet;
	descriptorWrites[10].dstBinding = 10;
	descriptorWrites[10].dstArrayElement = 0;
	descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[10].descriptorCount = 1;
	descriptorWrites[10].pImageInfo = &GIMapImageInfo;

	descriptorWrites[11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[11].dstSet = descriptorSet;
	descriptorWrites[11].dstBinding = 11;
	descriptorWrites[11].dstArrayElement = 0;
	descriptorWrites[11].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[11].descriptorCount = 1;
	descriptorWrites[11].pBufferInfo = &optionInfo;


	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}


void DebugDisplayMaterial::createDescriptorSetLayout()
{
	//BasicColorRenderTarget
	VkDescriptorSetLayoutBinding BasicColorRenderTargetLayoutBinding = {};
	BasicColorRenderTargetLayoutBinding.binding = 0;
	BasicColorRenderTargetLayoutBinding.descriptorCount = 1;
	BasicColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	BasicColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	BasicColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//SpecColorRenderTarget
	VkDescriptorSetLayoutBinding SpecColorRenderTargetLayoutBinding = {};
	SpecColorRenderTargetLayoutBinding.binding = 1;
	SpecColorRenderTargetLayoutBinding.descriptorCount = 1;
	SpecColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SpecColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SpecColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//NormalColorRenderTarget
	VkDescriptorSetLayoutBinding NormalColorRenderTargetLayoutBinding = {};
	NormalColorRenderTargetLayoutBinding.binding = 2;
	NormalColorRenderTargetLayoutBinding.descriptorCount = 1;
	NormalColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	NormalColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	NormalColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//EmissiveColorRenderTarget
	VkDescriptorSetLayoutBinding EmissiveColorRenderTargetLayoutBinding = {};
	EmissiveColorRenderTargetLayoutBinding.binding = 3;
	EmissiveColorRenderTargetLayoutBinding.descriptorCount = 1;
	EmissiveColorRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	EmissiveColorRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	EmissiveColorRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 4;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 5;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding DepthTextureLayoutBinding = {};
	DepthTextureLayoutBinding.binding = 6;
	DepthTextureLayoutBinding.descriptorCount = 1;
	DepthTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DepthTextureLayoutBinding.pImmutableSamplers = nullptr;
	DepthTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//GI
	VkDescriptorSetLayoutBinding GILayoutBinding = {};
	GILayoutBinding.binding = 7;
	GILayoutBinding.descriptorCount = 1;
	GILayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	GILayoutBinding.pImmutableSamplers = nullptr;
	GILayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Shadow
	VkDescriptorSetLayoutBinding ShadowMapLayoutBinding = {};
	ShadowMapLayoutBinding.binding = 8;
	ShadowMapLayoutBinding.descriptorCount = 1;
	ShadowMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ShadowMapLayoutBinding.pImmutableSamplers = nullptr;
	ShadowMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::array<VkDescriptorSetLayoutBinding, 9> bindings = { BasicColorRenderTargetLayoutBinding, SpecColorRenderTargetLayoutBinding, NormalColorRenderTargetLayoutBinding, EmissiveColorRenderTargetLayoutBinding,
		uboLayoutBinding, fuboLayoutBinding, DepthTextureLayoutBinding, GILayoutBinding, ShadowMapLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void DebugDisplayMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 9> poolSizes = {};
	poolSizes[BASIC_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[BASIC_COLOR].descriptorCount = 1;

	poolSizes[SPECULAR_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[SPECULAR_COLOR].descriptorCount = 1;

	poolSizes[NORMAL_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[NORMAL_COLOR].descriptorCount = 1;

	poolSizes[EMISSIVE_COLOR].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[EMISSIVE_COLOR].descriptorCount = 1;	

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[5].descriptorCount = 1;

	poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[6].descriptorCount = 1;

	poolSizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[7].descriptorCount = 1;

	poolSizes[8].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[8].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DebugDisplayMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	basicColorImageInfo.imageView = (*gBufferImageViews)[BASIC_COLOR];
	basicColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularColorImageInfo.imageView = (*gBufferImageViews)[SPECULAR_COLOR];
	specularColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalColorImageInfo.imageView = (*gBufferImageViews)[NORMAL_COLOR];
	normalColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveColorImageInfo.imageView = (*gBufferImageViews)[EMISSIVE_COLOR];
	emissiveColorImageInfo.sampler = textureSampler;
	
	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo dethImageInfo = {};
	dethImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dethImageInfo.imageView = DepthImageView;
	dethImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo GIImageInfo = {};
	GIImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	GIImageInfo.imageView = GIImageView;
	GIImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo shadowImageInfo = {};
	shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowImageInfo.imageView = shadowImageView;
	shadowImageInfo.sampler = textureSampler;


	std::array<VkWriteDescriptorSet, 9> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &dethImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &GIImageInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &shadowImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void DebugDisplayMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;

	viewport.width = static_cast<float>(extent.x) * widthScale;
	viewport.height = extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D _extent;
	_extent.width = (uint32_t)viewport.width;
	_extent.height = (uint32_t)viewport.height;

	scissor.extent = _extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//2nd Eye
	viewport.x += viewport.width * 4.0f;
	scissor.offset.x = static_cast<int32_t>(viewport.x);

	VkPipelineViewportStateCreateInfo viewportState2 = {};
	viewportState2.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState2.viewportCount = 1;
	viewportState2.pViewports = &viewport;
	viewportState2.scissorCount = 1;
	viewportState2.pScissors = &scissor;

	VkGraphicsPipelineCreateInfo pipelineInfo2 = {};
	pipelineInfo2.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo2.stageCount = 2;
	pipelineInfo2.pStages = shaderStages;
	pipelineInfo2.pVertexInputState = &vertexInputInfo;
	pipelineInfo2.pInputAssemblyState = &inputAssembly;
	pipelineInfo2.pViewportState = &viewportState2;
	pipelineInfo2.pRasterizationState = &rasterizer;
	pipelineInfo2.pMultisampleState = &multisampling;
	pipelineInfo2.pColorBlendState = &colorBlending;
	pipelineInfo2.layout = pipelineLayout;
	pipelineInfo2.renderPass = renderPass;
	pipelineInfo2.subpass = 0;
	pipelineInfo2.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo2.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo2, nullptr, &pipeline2) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline2!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void DebugDisplayMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo basicColorImageInfo = {};
	basicColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	basicColorImageInfo.imageView = (*gBufferImageViews)[BASIC_COLOR];
	basicColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo specularColorImageInfo = {};
	specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularColorImageInfo.imageView = (*gBufferImageViews)[SPECULAR_COLOR];
	specularColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalColorImageInfo = {};
	normalColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalColorImageInfo.imageView = (*gBufferImageViews)[NORMAL_COLOR];
	normalColorImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo emissiveColorImageInfo = {};
	emissiveColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveColorImageInfo.imageView = (*gBufferImageViews)[EMISSIVE_COLOR];
	emissiveColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo dethImageInfo = {};
	dethImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dethImageInfo.imageView = DepthImageView;
	dethImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo GIImageInfo = {};
	GIImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	GIImageInfo.imageView = GIImageView;
	GIImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo shadowImageInfo = {};
	shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowImageInfo.imageView = shadowImageView;
	shadowImageInfo.sampler = textureSampler;


	std::array<VkWriteDescriptorSet, 9> descriptorWrites = {};

	descriptorWrites[BASIC_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[BASIC_COLOR].dstSet = descriptorSet;
	descriptorWrites[BASIC_COLOR].dstBinding = 0;
	descriptorWrites[BASIC_COLOR].dstArrayElement = 0;
	descriptorWrites[BASIC_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[BASIC_COLOR].descriptorCount = 1;
	descriptorWrites[BASIC_COLOR].pImageInfo = &basicColorImageInfo;

	descriptorWrites[SPECULAR_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[SPECULAR_COLOR].dstSet = descriptorSet;
	descriptorWrites[SPECULAR_COLOR].dstBinding = 1;
	descriptorWrites[SPECULAR_COLOR].dstArrayElement = 0;
	descriptorWrites[SPECULAR_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[SPECULAR_COLOR].descriptorCount = 1;
	descriptorWrites[SPECULAR_COLOR].pImageInfo = &specularColorImageInfo;

	descriptorWrites[NORMAL_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[NORMAL_COLOR].dstSet = descriptorSet;
	descriptorWrites[NORMAL_COLOR].dstBinding = 2;
	descriptorWrites[NORMAL_COLOR].dstArrayElement = 0;
	descriptorWrites[NORMAL_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[NORMAL_COLOR].descriptorCount = 1;
	descriptorWrites[NORMAL_COLOR].pImageInfo = &normalColorImageInfo;

	descriptorWrites[EMISSIVE_COLOR].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[EMISSIVE_COLOR].dstSet = descriptorSet;
	descriptorWrites[EMISSIVE_COLOR].dstBinding = 3;
	descriptorWrites[EMISSIVE_COLOR].dstArrayElement = 0;
	descriptorWrites[EMISSIVE_COLOR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[EMISSIVE_COLOR].descriptorCount = 1;
	descriptorWrites[EMISSIVE_COLOR].pImageInfo = &emissiveColorImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &fragbufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &dethImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &GIImageInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &shadowImageInfo;


	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}


void FinalRenderingMaterial::createDescriptorSetLayout()
{
	//BasicColorRenderTarget
	VkDescriptorSetLayoutBinding SceneRenderTargetLayoutBinding = {};
	SceneRenderTargetLayoutBinding.binding = 0;
	SceneRenderTargetLayoutBinding.descriptorCount = 1;
	SceneRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SceneRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SceneRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 1;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 2;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { SceneRenderTargetLayoutBinding, uboLayoutBinding, fuboLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void FinalRenderingMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void FinalRenderingMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = finalImageView;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);


	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void FinalRenderingMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo sceneImageInfo = {};
	sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneImageInfo.imageView = finalImageView;
	sceneImageInfo.sampler = textureSampler;
	
	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void FinalRenderingMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;
	
	
	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}
	

	viewport.height = extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//2nd Eye
	viewport.x = viewport.width;
	scissor.offset.x = static_cast<int32_t>(viewport.x);

	VkPipelineViewportStateCreateInfo viewportState2 = {};
	viewportState2.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState2.viewportCount = 1;
	viewportState2.pViewports = &viewport;
	viewportState2.scissorCount = 1;
	viewportState2.pScissors = &scissor;

	VkGraphicsPipelineCreateInfo pipelineInfo2 = {};
	pipelineInfo2.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo2.stageCount = 2;
	pipelineInfo2.pStages = shaderStages;
	pipelineInfo2.pVertexInputState = &vertexInputInfo;
	pipelineInfo2.pInputAssemblyState = &inputAssembly;
	pipelineInfo2.pViewportState = &viewportState2;
	pipelineInfo2.pRasterizationState = &rasterizer;
	pipelineInfo2.pMultisampleState = &multisampling;
	pipelineInfo2.pColorBlendState = &colorBlending;
	pipelineInfo2.layout = pipelineLayout;
	pipelineInfo2.renderPass = renderPass;
	pipelineInfo2.subpass = 0;
	pipelineInfo2.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo2.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo2, nullptr, &pipeline2) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline2!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void VoxelConetracingMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding depthMapLayoutBinding = {};
	depthMapLayoutBinding.binding = 1;
	depthMapLayoutBinding.descriptorCount = 1;
	depthMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	depthMapLayoutBinding.pImmutableSamplers = nullptr;
	depthMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalMapLayoutBinding = {};
	normalMapLayoutBinding.binding = 2;
	normalMapLayoutBinding.descriptorCount = 1;
	normalMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalMapLayoutBinding.pImmutableSamplers = nullptr;
	normalMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding voInfoLayoutBinding = {};
	voInfoLayoutBinding.binding = 3;
	voInfoLayoutBinding.descriptorCount = 1;
	voInfoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	voInfoLayoutBinding.pImmutableSamplers = nullptr;
	voInfoLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3dImageLayoutBinding = {};
	albedo3dImageLayoutBinding.binding = 4;
	albedo3dImageLayoutBinding.descriptorCount = 1;
	albedo3dImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo3dImageLayoutBinding.pImmutableSamplers = nullptr;
	albedo3dImageLayoutBinding.stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3dImage01LayoutBinding = {};
	albedo3dImage01LayoutBinding.binding = 5;
	albedo3dImage01LayoutBinding.descriptorCount = 1;
	albedo3dImage01LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo3dImage01LayoutBinding.pImmutableSamplers = nullptr;
	albedo3dImage01LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3dImage02LayoutBinding = {};
	albedo3dImage02LayoutBinding.binding = 6;
	albedo3dImage02LayoutBinding.descriptorCount = 1;
	albedo3dImage02LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo3dImage02LayoutBinding.pImmutableSamplers = nullptr;
	albedo3dImage02LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3dImage03LayoutBinding = {};
	albedo3dImage03LayoutBinding.binding = 7;
	albedo3dImage03LayoutBinding.descriptorCount = 1;
	albedo3dImage03LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo3dImage03LayoutBinding.pImmutableSamplers = nullptr;
	albedo3dImage03LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedo3dImage04LayoutBinding = {};
	albedo3dImage04LayoutBinding.binding = 8;
	albedo3dImage04LayoutBinding.descriptorCount = 1;
	albedo3dImage04LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo3dImage04LayoutBinding.pImmutableSamplers = nullptr;
	albedo3dImage04LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//shadow
	VkDescriptorSetLayoutBinding ShadowMapLB = {};
	ShadowMapLB.binding = 9;
	ShadowMapLB.descriptorCount = 1;
	ShadowMapLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ShadowMapLB.pImmutableSamplers = nullptr;
	ShadowMapLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding shadowUBOLB = {};
	shadowUBOLB.binding = 10;
	shadowUBOLB.descriptorCount = 1;
	shadowUBOLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadowUBOLB.pImmutableSamplers = nullptr;
	shadowUBOLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding directionalLayoutBinding = {};
	directionalLayoutBinding.binding = 11;
	directionalLayoutBinding.descriptorCount = 1;
	directionalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	directionalLayoutBinding.pImmutableSamplers = nullptr;
	directionalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding SpecularMapLB = {};
	SpecularMapLB.binding = 12;
	SpecularMapLB.descriptorCount = 1;
	SpecularMapLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SpecularMapLB.pImmutableSamplers = nullptr;
	SpecularMapLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::array<VkDescriptorSetLayoutBinding, 13> bindings = { uboLayoutBinding, depthMapLayoutBinding, normalMapLayoutBinding, voInfoLayoutBinding, albedo3dImageLayoutBinding,
		albedo3dImage01LayoutBinding, albedo3dImage02LayoutBinding, albedo3dImage03LayoutBinding, albedo3dImage04LayoutBinding, ShadowMapLB, shadowUBOLB, directionalLayoutBinding,
		SpecularMapLB
	};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VoxelConetracingMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 13> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = 1;

	poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[4].descriptorCount = 1;

	poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[5].descriptorCount = 1;

	poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[6].descriptorCount = 1;

	poolSizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[7].descriptorCount = 1;

	poolSizes[8].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[8].descriptorCount = 1;

	poolSizes[9].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[9].descriptorCount = 1;

	poolSizes[10].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[10].descriptorCount = 1;

	poolSizes[11].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[11].descriptorCount = 1;

	poolSizes[12].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[12].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VoxelConetracingMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);
	

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	
	VkDescriptorImageInfo deapthMapInfo = {};
	deapthMapInfo.imageView = depthImageView;
	deapthMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deapthMapInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalMapInfo = {};
	normalMapInfo.imageView = normalImageView;
	normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalMapInfo.sampler = textureSampler;

	VkDescriptorBufferInfo voxelInfo = {};
	voxelInfo.buffer = VoxelInfoBuffer;
	voxelInfo.offset = 0;
	voxelInfo.range = sizeof(VoxelInfo);


	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImageInfo.imageView = (*albedo3DImageViewSet)[0];
	albedo3DImageInfo.sampler = textureSampler;


	VkDescriptorImageInfo albedo3DImage01Info = {};
	albedo3DImage01Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage01Info.imageView = (*albedo3DImageViewSet)[1];
	albedo3DImage01Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage02Info = {};
	albedo3DImage02Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage02Info.imageView = (*albedo3DImageViewSet)[2];
	albedo3DImage02Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage03Info = {};
	albedo3DImage03Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage03Info.imageView = (*albedo3DImageViewSet)[3];
	albedo3DImage03Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage04Info = {};
	albedo3DImage04Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage04Info.imageView = (*albedo3DImageViewSet)[4];
	albedo3DImage04Info.sampler = textureSampler;

	//shadow
	VkDescriptorImageInfo shadowMapImageInfo = {};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapView;
	shadowMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo shadowUBOInfo = {};
	shadowUBOInfo.buffer = shadowConstantBuffer;
	shadowUBOInfo.offset = 0;
	shadowUBOInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorBufferInfo directionalLightInfo = {};
	directionalLightInfo.buffer = directionalLightBuffer;
	directionalLightInfo.offset = 0;
	directionalLightInfo.range = sizeof(DirectionalLight) * (*pDirectionLights).size();

	VkDescriptorImageInfo specularMapInfo = {};
	specularMapInfo.imageView = specularMapView;
	specularMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularMapInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 13> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &deapthMapInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &normalMapInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &voxelInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pImageInfo = &albedo3DImageInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pImageInfo = &albedo3DImage01Info;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &albedo3DImage02Info;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &albedo3DImage03Info;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &albedo3DImage04Info;

	descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[9].dstSet = descriptorSet;
	descriptorWrites[9].dstBinding = 9;
	descriptorWrites[9].dstArrayElement = 0;
	descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[9].descriptorCount = 1;
	descriptorWrites[9].pImageInfo = &shadowMapImageInfo;

	descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[10].dstSet = descriptorSet;
	descriptorWrites[10].dstBinding = 10;
	descriptorWrites[10].dstArrayElement = 0;
	descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[10].descriptorCount = 1;
	descriptorWrites[10].pBufferInfo = &shadowUBOInfo;

	descriptorWrites[11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[11].dstSet = descriptorSet;
	descriptorWrites[11].dstBinding = 11;
	descriptorWrites[11].dstArrayElement = 0;
	descriptorWrites[11].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[11].descriptorCount = 1;
	descriptorWrites[11].pBufferInfo = &directionalLightInfo;

	descriptorWrites[12].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[12].dstSet = descriptorSet;
	descriptorWrites[12].dstBinding = 12;
	descriptorWrites[12].dstArrayElement = 0;
	descriptorWrites[12].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[12].descriptorCount = 1;
	descriptorWrites[12].pImageInfo = &specularMapInfo;


	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void VoxelConetracingMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	
	VkDescriptorImageInfo deapthMapInfo = {};
	deapthMapInfo.imageView = depthImageView;
	deapthMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deapthMapInfo.sampler = textureSampler;

	VkDescriptorImageInfo normalMapInfo = {};
	normalMapInfo.imageView = normalImageView;
	normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalMapInfo.sampler = textureSampler;

	VkDescriptorBufferInfo voxelInfo = {};
	voxelInfo.buffer = VoxelInfoBuffer;
	voxelInfo.offset = 0;
	voxelInfo.range = sizeof(VoxelInfo);

	VkDescriptorImageInfo albedo3DImageInfo = {};
	albedo3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImageInfo.imageView = (*albedo3DImageViewSet)[0];
	albedo3DImageInfo.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage01Info = {};
	albedo3DImage01Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage01Info.imageView = (*albedo3DImageViewSet)[1];
	albedo3DImage01Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage02Info = {};
	albedo3DImage02Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage02Info.imageView = (*albedo3DImageViewSet)[2];
	albedo3DImage02Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage03Info = {};
	albedo3DImage03Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage03Info.imageView = (*albedo3DImageViewSet)[3];
	albedo3DImage03Info.sampler = textureSampler;

	VkDescriptorImageInfo albedo3DImage04Info = {};
	albedo3DImage04Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo3DImage04Info.imageView = (*albedo3DImageViewSet)[4];
	albedo3DImage04Info.sampler = textureSampler;

	//shadow
	VkDescriptorImageInfo shadowMapImageInfo = {};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapView;
	shadowMapImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo shadowUBOInfo = {};
	shadowUBOInfo.buffer = shadowConstantBuffer;
	shadowUBOInfo.offset = 0;
	shadowUBOInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorBufferInfo directionalLightInfo = {};
	directionalLightInfo.buffer = directionalLightBuffer;
	directionalLightInfo.offset = 0;
	directionalLightInfo.range = sizeof(DirectionalLight) * (*pDirectionLights).size();

	VkDescriptorImageInfo specularMapInfo = {};
	specularMapInfo.imageView = specularMapView;
	specularMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularMapInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 13> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &deapthMapInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &normalMapInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &voxelInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pImageInfo = &albedo3DImageInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSet;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pImageInfo = &albedo3DImage01Info;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSet;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &albedo3DImage02Info;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSet;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &albedo3DImage03Info;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = descriptorSet;
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &albedo3DImage04Info;

	descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[9].dstSet = descriptorSet;
	descriptorWrites[9].dstBinding = 9;
	descriptorWrites[9].dstArrayElement = 0;
	descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[9].descriptorCount = 1;
	descriptorWrites[9].pImageInfo = &shadowMapImageInfo;

	descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[10].dstSet = descriptorSet;
	descriptorWrites[10].dstBinding = 10;
	descriptorWrites[10].dstArrayElement = 0;
	descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[10].descriptorCount = 1;
	descriptorWrites[10].pBufferInfo = &shadowUBOInfo;

	descriptorWrites[11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[11].dstSet = descriptorSet;
	descriptorWrites[11].dstBinding = 11;
	descriptorWrites[11].dstArrayElement = 0;
	descriptorWrites[11].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[11].descriptorCount = 1;
	descriptorWrites[11].pBufferInfo = &directionalLightInfo;

	descriptorWrites[12].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[12].dstSet = descriptorSet;
	descriptorWrites[12].dstBinding = 12;
	descriptorWrites[12].dstArrayElement = 0;
	descriptorWrites[12].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[12].descriptorCount = 1;
	descriptorWrites[12].pImageInfo = &specularMapInfo;


	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void VoxelConetracingMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;


	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}


	viewport.height = Extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}



void HDRHighlightMaterial::createDescriptorSetLayout()
{
	//BasicColorRenderTarget
	VkDescriptorSetLayoutBinding SceneRenderTargetLayoutBinding = {};
	SceneRenderTargetLayoutBinding.binding = 0;
	SceneRenderTargetLayoutBinding.descriptorCount = 1;
	SceneRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SceneRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SceneRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 1;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 2;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { SceneRenderTargetLayoutBinding, uboLayoutBinding, fuboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void HDRHighlightMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void HDRHighlightMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = ImageViews;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);


	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void HDRHighlightMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo sceneImageInfo = {};
	sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneImageInfo.imageView = ImageViews;
	sceneImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void HDRHighlightMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;

	
	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}
	

	viewport.height = Extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}


void BlurMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding SceneRenderTargetLayoutBinding = {};
	SceneRenderTargetLayoutBinding.binding = 0;
	SceneRenderTargetLayoutBinding.descriptorCount = 1;
	SceneRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SceneRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SceneRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 1;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 2;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { SceneRenderTargetLayoutBinding, uboLayoutBinding, fuboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void BlurMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void BlurMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = ImageViews;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo blurbufferInfo = {};
	blurbufferInfo.buffer = blurUniformBuffer;
	blurbufferInfo.offset = 0;
	blurbufferInfo.range = sizeof(BlurUniformBufferObject);


	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &blurbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void BlurMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo sceneImageInfo = {};
	sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneImageInfo.imageView = ImageViews;
	sceneImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo blurbufferInfo = {};
	blurbufferInfo.buffer = blurUniformBuffer;
	blurbufferInfo.offset = 0;
	blurbufferInfo.range = sizeof(BlurUniformBufferObject);

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &blurbufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void BlurMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;

	//viewport.width = static_cast<float>()extent.x * widthScale;

	
	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale * 0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}
	

	viewport.height = Extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void ComputeBlurMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding SceneRenderTargetLayoutBinding = {};
	SceneRenderTargetLayoutBinding.binding = 0;
	SceneRenderTargetLayoutBinding.descriptorCount = 1;
	SceneRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SceneRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SceneRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 1;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 2;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding OutputTextureLayoutBinding = {};
	OutputTextureLayoutBinding.binding = 3;
	OutputTextureLayoutBinding.descriptorCount = 1;
	OutputTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	OutputTextureLayoutBinding.pImmutableSamplers = nullptr;
	OutputTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> bindings = { SceneRenderTargetLayoutBinding, uboLayoutBinding, fuboLayoutBinding, OutputTextureLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void ComputeBlurMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[3].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void ComputeBlurMaterial::createDescriptorSet()
{

	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = inputImageViews;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo blurbufferInfo = {};
	blurbufferInfo.buffer = blurUniformBuffer;
	blurbufferInfo.offset = 0;
	blurbufferInfo.range = sizeof(BlurUniformBufferObject);

	VkDescriptorImageInfo outputTextureInfo = {};
	outputTextureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputTextureInfo.imageView = outputImageViews;
	outputTextureInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &blurbufferInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &outputTextureInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void ComputeBlurMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = inputImageViews;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo blurbufferInfo = {};
	blurbufferInfo.buffer = blurUniformBuffer;
	blurbufferInfo.offset = 0;
	blurbufferInfo.range = sizeof(BlurUniformBufferObject);

	VkDescriptorImageInfo outputTextureInfo = {};
	outputTextureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputTextureInfo.imageView = outputImageViews;
	outputTextureInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &blurbufferInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &outputTextureInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void ComputeBlurMaterial::createComputePipeline()
{
	auto compShaderCode = readFile(computeShaderPath);
	
	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	//extent = Extent;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = NULL;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = pipelineLayout;
	computePipelineInfo.basePipelineHandle = 0;
	computePipelineInfo.basePipelineIndex = 0;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}



void LastPostProcessgMaterial::createDescriptorSetLayout()
{
	//BasicColorRenderTarget
	VkDescriptorSetLayoutBinding SceneRenderTargetLayoutBinding = {};
	SceneRenderTargetLayoutBinding.binding = 0;
	SceneRenderTargetLayoutBinding.descriptorCount = 1;
	SceneRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SceneRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	SceneRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 1;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding fuboLayoutBinding = {};
	fuboLayoutBinding.binding = 2;
	fuboLayoutBinding.descriptorCount = 1;
	fuboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fuboLayoutBinding.pImmutableSamplers = nullptr;
	fuboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding BloomRenderTargetLayoutBinding = {};
	BloomRenderTargetLayoutBinding.binding = 3;
	BloomRenderTargetLayoutBinding.descriptorCount = 1;
	BloomRenderTargetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	BloomRenderTargetLayoutBinding.pImmutableSamplers = nullptr;
	BloomRenderTargetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> bindings = { SceneRenderTargetLayoutBinding, uboLayoutBinding, fuboLayoutBinding, BloomRenderTargetLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void LastPostProcessgMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size()) - 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void LastPostProcessgMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);


	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = sceneImageView;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);


	VkDescriptorImageInfo bloomImageInfo = {};
	bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	bloomImageInfo.imageView = bloomImageView;
	bloomImageInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneColorImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &bloomImageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void LastPostProcessgMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkDescriptorImageInfo sceneImageInfo = {};
	sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneImageInfo.imageView = sceneImageView;
	sceneImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = uniformBuffer;
	fragbufferInfo.offset = 0;
	fragbufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo bloomImageInfo = {};
	bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	bloomImageInfo.imageView = bloomImageView;
	bloomImageInfo.sampler = textureSampler;

	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &sceneImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &fragbufferInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &bloomImageInfo;


	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
}

void LastPostProcessgMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;

	//viewport.width = static_cast<float>()extent.x * widthScale;

	
	if (vrMode)
	{
		viewport.width = static_cast<float>(extent.x) * widthScale *0.5f;
	}
	else
	{
		viewport.width = static_cast<float>(extent.x) * widthScale;
	}
	

	viewport.height = extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y)};

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = NULL;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}



void StandardShadowMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding ShadowConstantUniform = {};
	ShadowConstantUniform.binding = 0;
	ShadowConstantUniform.descriptorCount = 1;
	ShadowConstantUniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ShadowConstantUniform.pImmutableSamplers = nullptr;
	ShadowConstantUniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding objectUniform = {};
	objectUniform.binding = 1;
	objectUniform.descriptorCount = 1;
	objectUniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	objectUniform.pImmutableSamplers = nullptr;
	objectUniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ShadowConstantUniform, objectUniform };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void StandardShadowMaterial::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(poolSizes.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void StandardShadowMaterial::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo shadowBufferInfo = {};
	shadowBufferInfo.buffer = ShadowConstantBuffer;
	shadowBufferInfo.offset = 0;
	shadowBufferInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorBufferInfo objectBufferInfo = {};
	objectBufferInfo.buffer = objectUniformBuffer;
	objectBufferInfo.offset = 0;
	objectBufferInfo.range = sizeof(ObjectUniformBuffer);


	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &shadowBufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &objectBufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void StandardShadowMaterial::updateDescriptorSet()
{
	VkDescriptorBufferInfo shadowBufferInfo = {};
	shadowBufferInfo.buffer = ShadowConstantBuffer;
	shadowBufferInfo.offset = 0;
	shadowBufferInfo.range = sizeof(ShadowUniformBuffer);

	VkDescriptorBufferInfo objectBufferInfo = {};
	objectBufferInfo.buffer = objectUniformBuffer;
	objectBufferInfo.offset = 0;
	objectBufferInfo.range = sizeof(ObjectUniformBuffer);


	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &shadowBufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &objectBufferInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void StandardShadowMaterial::createGraphicsPipeline(glm::vec2 Extent, glm::vec2 ScreenOffset)
{
	auto vertShaderCode = readFile(vertexShaderPath);
	auto fragShaderCode = readFile(fragmentShaderPath);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	extent = Extent;

	VkViewport viewport = {};
	viewport.x = ScreenOffset.x;
	viewport.y = ScreenOffset.y;	
	
	viewport.width = static_cast<float>(extent.x) * widthScale;

	viewport.height = extent.y * heightScale;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { static_cast<int32_t>(ScreenOffset.x), static_cast<int32_t>(ScreenOffset.y) };

	VkExtent2D extent;
	extent.width = (uint32_t)viewport.width;
	extent.height = (uint32_t)viewport.height;

	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//  VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


	std::array<VkPipelineColorBlendAttachmentState, 1> Attachment = { colorBlendAttachment };


	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(Attachment.size());
	colorBlending.pAttachments = Attachment.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_TRUE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}