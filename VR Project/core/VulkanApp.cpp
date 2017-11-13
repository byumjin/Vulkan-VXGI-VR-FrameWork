#include "VulkanApp.h"

#include "../assets/AssetDatabase.h"


void onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width <= 0 || height <= 0)
		return;
	
	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

	camera.UpdateAspectRatio(float(width) / float(height));
	app->reCreateSwapChain();
}

void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			leftMouseDown = true;
			glfwGetCursorPos(window, &previousX, &previousY);
		}
		else if (action == GLFW_RELEASE) {
			leftMouseDown = false;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			rightMouseDown = true;
			glfwGetCursorPos(window, &previousX, &previousY);
		}
		else if (action == GLFW_RELEASE) {
			rightMouseDown = false;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		if (action == GLFW_PRESS) {
			middleMouseDown = true;
			glfwGetCursorPos(window, &previousX, &previousY);
		}
		else if (action == GLFW_RELEASE) {
			middleMouseDown = false;
		}
	}
}

void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition)
{
	if (rightMouseDown)
	{
		double sensitivity = 0.5;
		float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
		float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

		camera.UpdateOrbit(deltaX, deltaY, 0.0f);

		previousX = xPosition;
		previousY = yPosition;
	}
	else if (leftMouseDown)
	{
		double deltaZ = static_cast<float>((previousY - yPosition) * -0.05);

		camera.UpdateOrbit(0.0f, 0.0f, (float)deltaZ);

		previousY = yPosition;
	}
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	double deltaZ = static_cast<float>(yoffset * -0.5);

	camera.UpdateOrbit(0.0f, 0.0f, (float)deltaZ);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT || action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_W || key == GLFW_KEY_UP)
			camera.UpdatePosition(0.0f, 0.0f, (float)-2.0);
		else if(key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
			camera.UpdatePosition(0.0f, 0.0f, (float)2.0);
		else if(key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
			camera.UpdatePosition((float)-2.0f, 0.0f, 0.0f);
		else if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
			camera.UpdatePosition((float)2.0f, 0.0f, 0.0f);
		else if(key == GLFW_KEY_G)
		{
			bDeubDisply = !bDeubDisply;

			VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
			app->reCreateSwapChain();
		}
	}

	
}

VulkanApp::VulkanApp():WIDTH(800), HEIGHT(600), physicalDevice(VK_NULL_HANDLE), LayerCount(1)
{

}

VulkanApp::~VulkanApp()
{
}

void VulkanApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "VR Project", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, onWindowResized);
	glfwSetMouseButtonCallback(window, mouseDownCallback);
	glfwSetCursorPosCallback(window, mouseMoveCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetKeyCallback(window, keyboardCallback);
}

void VulkanApp::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();

	//01. Create Swapchains
	createSwapChain();


	//02. Create GlobalImagebuffers
	createGbuffers();
	createSceneBuffer();

	camera.setCamera(glm::vec3(0.0f, 3.0f, 15.0f), glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)WIDTH, (float)HEIGHT, NEAR_PLANE, FAR_PLANE);
	
	//03. Create CommandPool
	//[Stage] PreDraw 
	createDeferredCommandPool();


	//[Stage] Post-process
	PostProcess* sceneImageStage = new PostProcess;
	sceneImageStage->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f));
	sceneImageStage->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	sceneImageStage->createCommandPool();

	sceneStage = sceneImageStage;
	//lastPostProcess = sceneImageStage;

	
	PostProcess* HDRHighlightPostProcess = new PostProcess;

	HDRHighlightPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 8, glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));
	HDRHighlightPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HDRHighlightPostProcess->createCommandPool();	
	

	PostProcess* HorizontalBlurPostProcess = new PostProcess;
	HorizontalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));
	HorizontalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HorizontalBlurPostProcess->createCommandPool();
	
	
	PostProcess* VerticalBlurPostProcess = new PostProcess;
	VerticalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));
	VerticalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VerticalBlurPostProcess->createCommandPool();

	PostProcess* LastPostProcess = new PostProcess;
	LastPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f));
	LastPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	LastPostProcess->createCommandPool();

	theLastPostProcess = LastPostProcess;


	postProcessStages.push_back(sceneStage);
	postProcessStages.push_back(HDRHighlightPostProcess);
	postProcessStages.push_back(HorizontalBlurPostProcess);
	postProcessStages.push_back(VerticalBlurPostProcess);
	postProcessStages.push_back(LastPostProcess);

	//[Stage] Frame buffer
	createFrameBufferCommandPool();
	

	
	//03. Load Assets

	//[Lights]
	DirectionalLight DL01;
	DL01.lightInfo.lightColor = glm::vec4(1.0, 1.0, 1.0, 2.0);
	DL01.lightInfo.lightPosition = glm::vec4(0.0);
	DL01.lightDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
	directionLights.push_back(DL01);



	AssetDatabase::GetInstance();

	//[Geometries]
	AssetDatabase::SetDevice(device, physicalDevice, deferredCommandPool, objectDrawQueue);
	
	AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Johanna.obj");
	AssetDatabase::GetInstance()->geoList.push_back("objects/Johanna.obj");

	AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Chromie.obj");
	AssetDatabase::GetInstance()->geoList.push_back("objects/Chromie.obj");
	

	AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Cerberus/Cerberus.obj");
	AssetDatabase::GetInstance()->geoList.push_back("objects/Cerberus/Cerberus.obj");

	//[Textures] 
	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_diff.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_d3crusaderf_base_diff.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_spec.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_d3crusaderf_base_spec.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_norm.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_d3crusaderf_base_norm.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_emis.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_d3crusaderf_base_emis.tga");
	
	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_diff.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_chromie_ultimate_diff.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_spec.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_chromie_ultimate_spec.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_norm.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_chromie_ultimate_norm.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_emis.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/storm_hero_chromie_ultimate_emis.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_A.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/Cerberus/Cerberus_A.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_S.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/Cerberus/Cerberus_S.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_N.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/Cerberus/Cerberus_N.tga");

	AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_E.tga");
	AssetDatabase::GetInstance()->textureList.push_back("textures/Cerberus/Cerberus_E.tga");




	//[Materials]
	//Object Materials
	ObjectDrawMaterial* pMat = new ObjectDrawMaterial;
	pMat->LoadFromFilename(device, physicalDevice, deferredCommandPool, objectDrawQueue, "standard_material");
	pMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_diff.tga"));
	pMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_spec.tga"));
	pMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_norm.tga"));
	pMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_d3crusaderf_base_emis.tga"));
	pMat->setShaderPaths("shaders/shader.vert.spv", "shaders/shader.frag.spv", "");
	pMat->createDescriptorSet();

	materialManager.push_back(pMat);

	ObjectDrawMaterial* pMat2 = new ObjectDrawMaterial;
	pMat2->LoadFromFilename(device, physicalDevice, deferredCommandPool, objectDrawQueue, "standard_material2");
	pMat2->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_diff.tga"));
	pMat2->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_spec.tga"));
	pMat2->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_norm.tga"));
	pMat2->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/storm_hero_chromie_ultimate_emis.tga"));
	pMat2->setShaderPaths("shaders/shader.vert.spv", "shaders/shader.frag.spv", "");
	pMat2->createDescriptorSet();

	materialManager.push_back(pMat2);

	ObjectDrawMaterial* pMat3 = new ObjectDrawMaterial;
	pMat3->LoadFromFilename(device, physicalDevice, deferredCommandPool, objectDrawQueue, "standard_material3");
	pMat3->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_A.tga"));
	pMat3->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_S.tga"));
	pMat3->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_N.tga"));
	pMat3->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>("textures/Cerberus/Cerberus_E.tga"));
	pMat3->setShaderPaths("shaders/shader.vert.spv", "shaders/shader.frag.spv", "");
	pMat3->createDescriptorSet();

	materialManager.push_back(pMat3);

	//Global Materials
	lightingMaterial = new LightingMaterial;	
	lightingMaterial->setDirectionalLights(&directionLights);	
	lightingMaterial->LoadFromFilename(device, physicalDevice, sceneStage->commandPool, lightingQueue, "lighting_material");
	lightingMaterial->creatDirectionalLightBuffer();
	lightingMaterial->setShaderPaths("shaders/lighting.vert.spv", "shaders/lighting.frag.spv", "");
	//Link
	sceneStage->material = lightingMaterial;
	

	debugDisplayMaterials.resize(NUM_DEBUGDISPLAY);
	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		debugDisplayMaterials[i] = new DebugDisplayMaterial;
		debugDisplayMaterials[i]->LoadFromFilename(device, physicalDevice, deferredCommandPool, lightingQueue, "debugDisplay_material");
		debugDisplayMaterials[i]->setShaderPaths("shaders/debug.vert.spv", "shaders/debug" + convertToString((int)i) + ".frag.spv", "");
	}

	//Post Process Materials
	hdrHighlightMaterial = new HDRHighlightMaterial;
	hdrHighlightMaterial->LoadFromFilename(device, physicalDevice, HDRHighlightPostProcess->commandPool, postProcessQueue, "hdrHighlight_material");
	hdrHighlightMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/HDRHighlight.frag.spv", "");
	hdrHighlightMaterial->setScreenScale(glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));

	//Link
	HDRHighlightPostProcess->material = hdrHighlightMaterial;

	horizontalMaterial = new BlurMaterial;
	horizontalMaterial->LoadFromFilename(device, physicalDevice, HorizontalBlurPostProcess->commandPool, postProcessQueue, "horizontalBlur_material");
	horizontalMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/horizontalBlur.frag.spv", "");
	horizontalMaterial->setScreenScale(glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));

	HorizontalBlurPostProcess->material = horizontalMaterial;
	
	verticalMaterial = new BlurMaterial;
	verticalMaterial->LoadFromFilename(device, physicalDevice, VerticalBlurPostProcess->commandPool, postProcessQueue, "verticalBlur_material");
	verticalMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/verticalBlur.frag.spv", "");
	verticalMaterial->setScreenScale(glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));
	
	VerticalBlurPostProcess->material = verticalMaterial;

	lastPostProcessMaterial = new LastPostProcessgMaterial;
	lastPostProcessMaterial->LoadFromFilename(device, physicalDevice, LastPostProcess->commandPool, postProcessQueue, "lastPostProcess_material");
	lastPostProcessMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/lastPostProcess.frag.spv", "");

	LastPostProcess->material = lastPostProcessMaterial;

	

	//Frame Buffer Materials
	frameBufferMaterial = new FinalRenderingMaterial;
	frameBufferMaterial->LoadFromFilename(device, physicalDevice, frameBufferCommandPool, presentQueue, "frameDisplay_material");
	frameBufferMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/scene.frag.spv", "");





	//Create Objects
	
	Object obj01;
	obj01.initiation("Johanna", AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Johanna.obj"));
	obj01.connectMaterial(pMat);
	objectManager.push_back(obj01);
	

	Object obj02;
	obj02.initiation("Chromie", AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Chromie.obj"));
	obj02.scale = glm::vec3(0.2f);
	obj02.position = glm::vec3(0.0, 0.0, 4.0);
	obj02.update();
	obj02.connectMaterial(pMat2);
	objectManager.push_back(obj02);
	

	Object obj03;
	obj03.initiation("Cerberus", AssetDatabase::GetInstance()->LoadAsset<Geo>("objects/Cerberus/Cerberus.obj"));
	obj03.scale = glm::vec3(10.0f);
	obj03.update();
	obj03.connectMaterial(pMat3);
	objectManager.push_back(obj03);


	offScreenPlane = new singleTriangular;
	offScreenPlane->LoadFromFilename(device, physicalDevice, frameBufferCommandPool, lightingQueue, "offScreenPlane");

	debugDisplayPlane = new singleQuadral;
	debugDisplayPlane->LoadFromFilename(device, physicalDevice, frameBufferCommandPool, lightingQueue, "debugDisplayPlane");
	
	offScreenPlaneforPostProcess = new singleTriangular;
	offScreenPlaneforPostProcess->LoadFromFilename(device, physicalDevice,  sceneImageStage->commandPool, postProcessQueue, "offScreenPlaneforPostProcess");



	//04. Create Image views
	createImageViews();


	//05. Create Renderpass
	createDeferredRenderPass();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->createRenderPass();
	}

	createFrameBufferRenderPass();


	//06. Create Depth
	createDepthResources();


	//07. Create FrameBuffers
	createDeferredFramebuffer();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->createFramebuffer();
	}

	createFramebuffers();



	//08. Create GraphicsPipelines
	//[objects]
	for (size_t i = 0; i < materialManager.size(); i++)
	{
		Material* tempMat = materialManager[i];

		ObjectDrawMaterial* tempObjectDrawMaterial = dynamic_cast<ObjectDrawMaterial *>(tempMat);

		if (tempObjectDrawMaterial != NULL)
		{
			materialManager[i]->connectRenderPass(deferredRenderPass);
			materialManager[i]->createGraphicsPipeline(swapChainExtent);

			continue;
		}
	}

	//[global]
	lightingMaterial->setGbuffers(&gBufferImageViews, depthImageView);
	lightingMaterial->createDescriptorSet();
	lightingMaterial->connectRenderPass(sceneStage->renderPass);
	lightingMaterial->createGraphicsPipeline(swapChainExtent);

	//[postProcess]
	hdrHighlightMaterial->setImageViews(sceneStage->outputImageView, depthImageView);
	hdrHighlightMaterial->createDescriptorSet();
	hdrHighlightMaterial->connectRenderPass(HDRHighlightPostProcess->renderPass);
	hdrHighlightMaterial->createGraphicsPipeline(glm::vec2(HDRHighlightPostProcess->pExtent2D->width, HDRHighlightPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));

	horizontalMaterial->setImageViews(HDRHighlightPostProcess->outputImageView, depthImageView);
	horizontalMaterial->createDescriptorSet();
	horizontalMaterial->connectRenderPass(HorizontalBlurPostProcess->renderPass);
	horizontalMaterial->createGraphicsPipeline(glm::vec2(HorizontalBlurPostProcess->pExtent2D->width, HorizontalBlurPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));

	verticalMaterial->setImageViews(HorizontalBlurPostProcess->outputImageView, depthImageView);
	verticalMaterial->createDescriptorSet();
	verticalMaterial->connectRenderPass(VerticalBlurPostProcess->renderPass);
	verticalMaterial->createGraphicsPipeline(glm::vec2(VerticalBlurPostProcess->pExtent2D->width, VerticalBlurPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));
	
	lastPostProcessMaterial->setImageViews(sceneStage->outputImageView, VerticalBlurPostProcess->outputImageView, depthImageView);
	lastPostProcessMaterial->createDescriptorSet();
	lastPostProcessMaterial->connectRenderPass(LastPostProcess->renderPass);
	lastPostProcessMaterial->createGraphicsPipeline(glm::vec2(LastPostProcess->pExtent2D->width, LastPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));
	


	//[debug]
	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		debugDisplayMaterials[i]->setDubugBuffers(&gBufferImageViews, depthImageView, HDRHighlightPostProcess->outputImageView);
		debugDisplayMaterials[i]->createDescriptorSet();
		debugDisplayMaterials[i]->connectRenderPass(frameBufferRenderPass);
	}

	float debugWidth = std::ceilf(swapChainExtent.width * 0.25f);
	float debugHeight = std::ceilf(swapChainExtent.height * 0.25f);

	debugDisplayMaterials[0]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, 0.0));
	debugDisplayMaterials[1]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth, 0.0));
	debugDisplayMaterials[2]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 2.0, 0.0));
	debugDisplayMaterials[3]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, 0.0));
	debugDisplayMaterials[4]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight));
	debugDisplayMaterials[5]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight * 2.0));
	debugDisplayMaterials[6]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight));
	debugDisplayMaterials[7]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight * 2.0));
	debugDisplayMaterials[8]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight * 3.0));
	debugDisplayMaterials[9]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth, debugHeight * 3.0));
	debugDisplayMaterials[10]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 2.0, debugHeight * 3.0));
	debugDisplayMaterials[11]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight * 3.0));
	

	//[framebuffer]
	frameBufferMaterial->setImageViews(theLastPostProcess->outputImageView, depthImageView);
	frameBufferMaterial->createDescriptorSet();
	frameBufferMaterial->connectRenderPass(frameBufferRenderPass);
	frameBufferMaterial->createGraphicsPipeline(glm::vec2(swapChainExtent.width, swapChainExtent.height), glm::vec2(0.0, 0.0));

	
	
	//09. Create CommandBuffers
	createDeferredCommandBuffers();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->offScreenPlane = offScreenPlane;
		postProcessStages[i]->createCommandBuffers();
	}

	createFrameBufferCommandBuffers();

	createSemaphores();
	
	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->createSemaphore();
	}
}

void VulkanApp::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VR Project";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

bool VulkanApp::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanApp::getRequiredExtensions()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

void  VulkanApp::createFramebufferDescriptorSetLayout()
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

void  VulkanApp::createFramebufferDescriptorPool()
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

void  VulkanApp::createFramebufferDescriptorSet()
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
	bufferInfo.buffer = frameBufferMaterial->uniformBuffer;// lastPostProcess->material->uniformBuffer;
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


	VkDescriptorImageInfo sceneColorImageInfo = {};
	sceneColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sceneColorImageInfo.imageView = theLastPostProcess->outputImageView;
	sceneColorImageInfo.sampler = textureSampler;

	VkDescriptorBufferInfo fragbufferInfo = {};
	fragbufferInfo.buffer = frameBufferMaterial->uniformBuffer;// lastPostProcess->material->uniformBuffer;
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


void VulkanApp::setupDebugCallback()
{
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

void VulkanApp::createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanApp::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool VulkanApp::isDeviceSuitable(VkPhysicalDevice device)
{
	/*
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
	*/

	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && supportedFeatures.samplerAnisotropy;
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}


void VulkanApp::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.deferredFamily, indices.graphicsFamily, indices.postProcessFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;


	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.deferredFamily, 0, &objectDrawQueue);
	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &lightingQueue);

	vkGetDeviceQueue(device, indices.postProcessFamily, 0, &postProcessQueue);

	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}


/*
QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.deferredFamily = i;
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}
*/

SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}	

	return details;
}


VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) 
	{
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanApp::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = LayerCount;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.deferredFamily, (uint32_t)indices.graphicsFamily,  (uint32_t)indices.postProcessFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	//Retrieving the swap chain images
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

void VulkanApp::reCreateSwapChain()
{
	vkDeviceWaitIdle(device);

	cleanUpSwapChain();

	createSwapChain();

	createGbuffers();
	createSceneBuffer();
	createImageViews();

	//sceneStage->createImages();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];		
		thisPostProcess->createImages();
	}
	

	//REDNER PASS
	createDeferredRenderPass();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];
		thisPostProcess->createRenderPass();
	}
	
	createFrameBufferRenderPass();



	createDepthResources();



	//Object Material
	for (size_t i = 0; i < materialManager.size(); i++)
	{
		Material* tempMat = materialManager[i];

		ObjectDrawMaterial* tempObjectDrawMaterial = dynamic_cast<ObjectDrawMaterial *>(tempMat);

		if (tempObjectDrawMaterial != NULL)
		{
			materialManager[i]->connectRenderPass(deferredRenderPass);
			materialManager[i]->createGraphicsPipeline(swapChainExtent);

			continue;
		}
	}
	
	lightingMaterial->setGbuffers(&gBufferImageViews, depthImageView);
	lightingMaterial->updateDescriptorSet();
	lightingMaterial->connectRenderPass(sceneStage->renderPass);
	lightingMaterial->createGraphicsPipeline(swapChainExtent);


		for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
		{
			debugDisplayMaterials[i]->setDubugBuffers(&gBufferImageViews, depthImageView, postProcessStages[3]->outputImageView);
			debugDisplayMaterials[i]->updateDescriptorSet();
			debugDisplayMaterials[i]->connectRenderPass(frameBufferRenderPass);
		}

		float debugWidth = swapChainExtent.width * 0.25f;
		float debugHeight = swapChainExtent.height * 0.25f;

		debugDisplayMaterials[0]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, 0.0));
		debugDisplayMaterials[1]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth, 0.0));
		debugDisplayMaterials[2]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 2.0, 0.0));
		debugDisplayMaterials[3]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, 0.0));
		debugDisplayMaterials[4]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight));
		debugDisplayMaterials[5]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight * 2.0));
		debugDisplayMaterials[6]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight));
		debugDisplayMaterials[7]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight * 2.0));
		debugDisplayMaterials[8]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(0.0, debugHeight * 3.0));
		debugDisplayMaterials[9]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth, debugHeight * 3.0));
		debugDisplayMaterials[10]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 2.0, debugHeight * 3.0));
		debugDisplayMaterials[11]->createGraphicsPipeline(glm::vec2(debugWidth, debugHeight), glm::vec2(debugWidth * 3.0, debugHeight * 3.0));
	

	
	//[postProcess]
	hdrHighlightMaterial->setImageViews(sceneStage->outputImageView, depthImageView);
	hdrHighlightMaterial->updateDescriptorSet();
	hdrHighlightMaterial->connectRenderPass(postProcessStages[1]->renderPass);
	hdrHighlightMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[1]->pExtent2D->width, postProcessStages[1]->pExtent2D->height), glm::vec2(0.0, 0.0));

	horizontalMaterial->setImageViews(postProcessStages[1]->outputImageView, depthImageView);
	horizontalMaterial->updateDescriptorSet();
	horizontalMaterial->connectRenderPass(postProcessStages[2]->renderPass);
	horizontalMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[2]->pExtent2D->width, postProcessStages[2]->pExtent2D->height), glm::vec2(0.0, 0.0));

	verticalMaterial->setImageViews(postProcessStages[2]->outputImageView, depthImageView);
	verticalMaterial->updateDescriptorSet();
	verticalMaterial->connectRenderPass(postProcessStages[3]->renderPass);
	verticalMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[3]->pExtent2D->width, postProcessStages[3]->pExtent2D->height), glm::vec2(0.0, 0.0));

	lastPostProcessMaterial->setImageViews(sceneStage->outputImageView, postProcessStages[3]->outputImageView, depthImageView);
	lastPostProcessMaterial->updateDescriptorSet();
	lastPostProcessMaterial->connectRenderPass(postProcessStages[4]->renderPass);
	lastPostProcessMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[4]->pExtent2D->width, postProcessStages[4]->pExtent2D->height), glm::vec2(0.0, 0.0));
	

	frameBufferMaterial->setImageViews(theLastPostProcess->outputImageView, depthImageView);
	frameBufferMaterial->updateDescriptorSet();
	frameBufferMaterial->connectRenderPass(frameBufferRenderPass);
	frameBufferMaterial->createGraphicsPipeline(glm::vec2(swapChainExtent.width, swapChainExtent.height), glm::vec2(0.0, 0.0));


	createDeferredFramebuffer();
	createDeferredCommandBuffers();
	
	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];

		thisPostProcess->createFramebuffer();
		thisPostProcess->createCommandBuffers();

	}
	createFramebuffers();
	createFrameBufferCommandBuffers();
}

void VulkanApp::createGbuffers()
{
	gBufferImages.resize(NUM_GBUFFERS);
	gBufferImageMemories.resize(NUM_GBUFFERS);

	for (uint32_t i = 0; i < gBufferImages.size(); i++)
	{
		
		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImages[i], gBufferImageMemories[i]);

	}
}

void VulkanApp::createSceneBuffer()
{
	createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sceneImage, sceneImageMemories);
}

void VulkanApp::createImageViews()
{
	gBufferImageViews.resize(gBufferImages.size());

	//G-buffers
	for (uint32_t i = 0; i < gBufferImages.size(); i++)
	{
		gBufferImageViews[i] = createImageView(gBufferImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	sceneImageView = createImageView(sceneImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	swapChainImageViews.resize(swapChainImages.size());	

	for (uint32_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
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


VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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

VkCommandBuffer VulkanApp::beginSingleTimeCommands(VkCommandPool commandPool)
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

void VulkanApp::endSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue)
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

void VulkanApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool)
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

	endSingleTimeCommands(commandPool, commandBuffer, objectDrawQueue);
}

void VulkanApp::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();

	createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, deferredCommandPool);
}

VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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


uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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


VkFormat VulkanApp::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VulkanApp::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanApp::createFrameBufferRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Subpasses and attachment references
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = NULL;
	
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBufferRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanApp::createDeferredRenderPass()
{
	
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription specColorAttachment = {};
	specColorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	specColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	specColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	specColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	specColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	specColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	specColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	specColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalColorAttachment = {};
	normalColorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription emissiveColorAttachment = {};
	emissiveColorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	emissiveColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	emissiveColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	emissiveColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	emissiveColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	emissiveColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	emissiveColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	emissiveColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Subpasses and attachment references
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference specColorAttachmentRef = {};
	specColorAttachmentRef.attachment = 1;
	specColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference normalColorAttachmentRef = {};
	normalColorAttachmentRef.attachment = 2;
	normalColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference emissiveColorAttachmentRef = {};
	emissiveColorAttachmentRef.attachment = 3;
	emissiveColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	std::array<VkAttachmentReference, 4> gBuffersAttachmentRef = { colorAttachmentRef, specColorAttachmentRef, normalColorAttachmentRef, emissiveColorAttachmentRef };

	
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
	depthAttachmentRef.attachment = static_cast<uint32_t>(gBuffersAttachmentRef.size());
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(gBuffersAttachmentRef.size());
	subpass.pColorAttachments = gBuffersAttachmentRef.data();
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	/*
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	*/

	// Use subpass dependencies for attachment layput transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 5> attachments = { colorAttachment, specColorAttachment, normalColorAttachment, emissiveColorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &deferredRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}




void VulkanApp::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		//std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageView};
		std::array<VkImageView, 1> attachments = { swapChainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = frameBufferRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = LayerCount;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanApp::createDeferredFramebuffer()
{
	std::array<VkImageView, NUM_GBUFFERS + 1> attachments = { gBufferImageViews[BASIC_COLOR], gBufferImageViews[SPECULAR_COLOR], gBufferImageViews[NORMAL_COLOR], gBufferImageViews[EMISSIVE_COLOR], depthImageView };

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = deferredRenderPass;
	fbufCreateInfo.pAttachments = attachments.data();
	fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbufCreateInfo.width = swapChainExtent.width;
	fbufCreateInfo.height = swapChainExtent.height;
	fbufCreateInfo.layers = LayerCount;

	if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &deferredFrameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}




void VulkanApp::createDeferredCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.deferredFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &deferredCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void VulkanApp::createDeferredCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = deferredCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &deferredCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(deferredCommandBuffer, &beginInfo);

	std::array<VkClearValue, NUM_GBUFFERS + 1> clearValues = {};
	clearValues[BASIC_COLOR].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[SPECULAR_COLOR].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[NORMAL_COLOR].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[EMISSIVE_COLOR].color = { 0.0f, 0.0f, 0.0f, 0.0f };

	clearValues[NUM_GBUFFERS].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = deferredRenderPass;
	renderPassInfo.framebuffer = deferredFrameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(deferredCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	for (size_t j = 0; j < objectManager.size(); j++)
	{
		Object thisObject = objectManager[j];

		vkCmdBindPipeline(deferredCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thisObject.material->pipeline);

		VkBuffer vertexBuffers[] = { thisObject.geo->vertexBuffer };
		VkBuffer indexBuffer = thisObject.geo->indexBuffer;
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindDescriptorSets(deferredCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thisObject.material->pipelineLayout, 0, 1, &thisObject.material->descriptorSet, 0, nullptr);

		vkCmdBindVertexBuffers(deferredCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(deferredCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(deferredCommandBuffer, static_cast<uint32_t>(thisObject.geo->indices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(deferredCommandBuffer);

	if (vkEndCommandBuffer(deferredCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanApp::createFrameBufferCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &frameBufferCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}


void VulkanApp::createFrameBufferCommandBuffers()
{
	frameBufferCommandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = frameBufferCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)frameBufferCommandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, frameBufferCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < frameBufferCommandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(frameBufferCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = frameBufferRenderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color = { 0.0f, 0.678431f, 0.902f, 1.0f };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(frameBufferCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(frameBufferCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, frameBufferMaterial->pipeline);

		VkBuffer vertexBuffers[] = { offScreenPlane->vertexBuffer };
		VkBuffer indexBuffer = offScreenPlane->indexBuffer;
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindDescriptorSets(frameBufferCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, frameBufferMaterial->pipelineLayout, 0, 1, &frameBufferMaterial->descriptorSet, 0, nullptr);

		vkCmdBindVertexBuffers(frameBufferCommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(frameBufferCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(frameBufferCommandBuffers[i], static_cast<uint32_t>(offScreenPlane->indices.size()), 1, 0, 0, 0);

		if (bDeubDisply)
		{
			for (size_t k = 0; k < NUM_DEBUGDISPLAY; k++)
			{
				vkCmdBindPipeline(frameBufferCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, debugDisplayMaterials[k]->pipeline);

				VkBuffer vertexBuffers[] = { debugDisplayPlane->vertexBuffer };
				VkBuffer indexBuffer = debugDisplayPlane->indexBuffer;
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindDescriptorSets(frameBufferCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, debugDisplayMaterials[k]->pipelineLayout, 0, 1, &debugDisplayMaterials[k]->descriptorSet, 0, nullptr);

				vkCmdBindVertexBuffers(frameBufferCommandBuffers[i], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(frameBufferCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(frameBufferCommandBuffers[i], static_cast<uint32_t>(debugDisplayPlane->indices.size()), 1, 0, 0, 0);
			}
		}

		vkCmdEndRenderPass(frameBufferCommandBuffers[i]);

		if (vkEndCommandBuffer(frameBufferCommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void VulkanApp::drawFrame()
{

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), objectDrawSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		reCreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//objectDrawQueue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore firstWaitSemaphores[] = { objectDrawSemaphore };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = firstWaitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &deferredCommandBuffer;
	
	VkSemaphore firstSignalSemaphores[] = { imageAvailableSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = firstSignalSemaphores;

	if (vkQueueSubmit(objectDrawQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//postProcessQueue
	VkSemaphore prevSemaphore = imageAvailableSemaphore;
	VkSemaphore currentSemaphore;

	for(size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &prevSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &thisPostProcess->commandBuffer;

		currentSemaphore = thisPostProcess->postProcessSemaphore;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &currentSemaphore;

		if (vkQueueSubmit(thisPostProcess->material->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		prevSemaphore = currentSemaphore;
	}



	//frameQueue
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &currentSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frameBufferCommandBuffers[imageIndex];

	VkSemaphore postProcessSignalSemaphores[] = { postProcessSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = postProcessSignalSemaphores;

	if (vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//presentQueue
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = postProcessSignalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		reCreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(presentQueue);
	
}

void VulkanApp::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &objectDrawSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &postProcessSemaphore) != VK_SUCCESS
		
		) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

void VulkanApp::run()
{
	initWindow();	
	initVulkan();
	mainLoop();
	cleanUp();
}

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		currentTime = (double)time(NULL);

		fpstracker++;

		if (currentTime - oldTime >= 1) {

			fps = (int)(fpstracker / (currentTime - oldTime));
			fpstracker = 0;
			oldTime = currentTime;

			std::string title = "VR Project | " + convertToString(fps) + " fps | " + convertToString(1000.0 / (double)fps) + " ms";
			glfwSetWindowTitle(window, title.c_str());
		}

		updateUniformBuffers();

		drawFrame();
	}

	vkDeviceWaitIdle(device);
}

void VulkanApp::updateUniformBuffers()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() * 0.001f;

	UniformBufferObject ubo = {};
	
	ubo.modelMat = glm::mat4(1.0);
	ubo.viewMat = camera.viewMat;
	ubo.projMat = camera.projMat;
	ubo.viewProjMat = camera.viewProjMat;
	ubo.InvViewProjMat = camera.InvViewProjMat;
	ubo.modelViewProjMat = ubo.viewProjMat;
	ubo.InvTransposeMat = ubo.modelMat;
	ubo.cameraWorldPos = camera.position;

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		Object thisObject = objectManager[i];


		thisObject.UpdateOrbit(time * 20.0f, 0.0f, 0.0f);

		ubo.modelMat = thisObject.modelMat;
		ubo.modelViewProjMat = camera.viewProjMat * thisObject.modelMat;

		glm::mat4 A = ubo.modelMat;
		A[3] = glm::vec4(0, 0, 0, 1);
		/*
		A[0][3] = 0.0;
		A[1][3] = 0.0;
		A[2][3] = 0.0;
		A[3][3] = 1.0;
		*/
		ubo.InvTransposeMat = glm::transpose(glm::inverse(A));			

		void* data;
		vkMapMemory(device, thisObject.material->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &ubo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, thisObject.material->uniformBufferMemory);
	}

	

	{
		UniformBufferObject offScreenUbo = {};

		offScreenUbo.modelMat = glm::mat4(1.0);
		offScreenUbo.viewMat = camera.viewMat;
		offScreenUbo.projMat = camera.projMat;
		offScreenUbo.viewProjMat = camera.viewProjMat;
		offScreenUbo.InvViewProjMat = camera.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = camera.position;

		offScreenPlane->updateVertexBuffer(offScreenUbo.InvViewProjMat);
		offScreenPlaneforPostProcess->updateVertexBuffer(offScreenUbo.InvViewProjMat);

		void* data;
		vkMapMemory(device, lightingMaterial->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &offScreenUbo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, lightingMaterial->uniformBufferMemory);
		
		void* directionLightsData;
		vkMapMemory(device, lightingMaterial->directionalLightBufferMemory, 0, sizeof(DirectionalLight) * directionLights.size(), 0, &directionLightsData);
		memcpy(directionLightsData, &directionLights[0], sizeof(DirectionalLight) * directionLights.size());
		vkUnmapMemory(device, lightingMaterial->directionalLightBufferMemory);
	}
	
	if (bDeubDisply)
	{
		{
			UniformBufferObject debugDisplayUbo = {};

			debugDisplayUbo.modelMat = glm::mat4(1.0);
			debugDisplayUbo.viewMat = camera.viewMat;
			debugDisplayUbo.projMat = camera.projMat;
			debugDisplayUbo.viewProjMat = camera.viewProjMat;
			debugDisplayUbo.InvViewProjMat = camera.InvViewProjMat;
			debugDisplayUbo.modelViewProjMat = debugDisplayUbo.viewProjMat;
			debugDisplayUbo.InvTransposeMat = debugDisplayUbo.modelMat;
			debugDisplayUbo.cameraWorldPos = camera.position;

			debugDisplayPlane->updateVertexBuffer(debugDisplayUbo.InvViewProjMat);

			for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
			{
				void* data;
				vkMapMemory(device, debugDisplayMaterials[i]->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
				memcpy(data, &debugDisplayUbo, sizeof(UniformBufferObject));
				vkUnmapMemory(device, debugDisplayMaterials[i]->uniformBufferMemory);
			}
		}
	}

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		UniformBufferObject offScreenUbo = {};

		offScreenUbo.modelMat = glm::mat4(1.0);
		offScreenUbo.viewMat = camera.viewMat;
		offScreenUbo.projMat = camera.projMat;
		offScreenUbo.viewProjMat = camera.viewProjMat;
		offScreenUbo.InvViewProjMat = camera.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = camera.position;

		offScreenPlaneforPostProcess->updateVertexBuffer(offScreenUbo.InvViewProjMat);

		void* data;
		vkMapMemory(device, postProcessStages[i]->material->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &offScreenUbo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, postProcessStages[i]->material->uniformBufferMemory);

		BlurMaterial* isBlurMat = dynamic_cast<BlurMaterial*>(postProcessStages[i]->material);

		if(isBlurMat != NULL)
		{
			BlurUniformBufferObject blurUbo;

			blurUbo.widthGap = isBlurMat->extent.x * isBlurMat->widthScale;
			blurUbo.heightGap = isBlurMat->extent.y * isBlurMat->heightScale;

			void* data;
			vkMapMemory(device, isBlurMat->blurUniformBufferMemory, 0, sizeof(BlurUniformBufferObject), 0, &data);
			memcpy(data, &blurUbo, sizeof(BlurUniformBufferObject));
			vkUnmapMemory(device, isBlurMat->blurUniformBufferMemory);
		}
	}

	{
		UniformBufferObject offScreenUbo = {};

		offScreenUbo.modelMat = glm::mat4(1.0);
		offScreenUbo.viewMat = camera.viewMat;
		offScreenUbo.projMat = camera.projMat;
		offScreenUbo.viewProjMat = camera.viewProjMat;
		offScreenUbo.InvViewProjMat = camera.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = camera.position;

		offScreenPlaneforPostProcess->updateVertexBuffer(offScreenUbo.InvViewProjMat);

		void* data;
		vkMapMemory(device, frameBufferMaterial->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &offScreenUbo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, frameBufferMaterial->uniformBufferMemory);
	}

}

void VulkanApp::cleanUpSwapChain()
{
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];
		thisPostProcess->cleanUp();
		

		vkDestroyFramebuffer(device, thisPostProcess->frameBuffer, nullptr);
		vkFreeCommandBuffers(device, thisPostProcess->commandPool, 1, &thisPostProcess->commandBuffer);
	}

	

	//delete Scene
	vkFreeMemory(device, sceneImageMemories, nullptr);
	vkDestroyImageView(device, sceneImageView, nullptr);
	vkDestroyImage(device, sceneImage, nullptr);

	//sceneStage->cleanUp();
	//vkDestroyFramebuffer(device, sceneStage->frameBuffer, nullptr);
	//vkFreeCommandBuffers(device, sceneStage->commandPool, 1, &sceneStage->commandBuffer);
	


	//delete Gbuffers
	for (size_t i = 0; i < gBufferImageMemories.size(); i++)
	{
		vkFreeMemory(device, gBufferImageMemories[i], nullptr);
	}

	for (size_t i = 0; i < gBufferImageViews.size(); i++)
	{
		vkDestroyImageView(device, gBufferImageViews[i], nullptr);
	}

	for (size_t i = 0; i < gBufferImages.size(); i++)
	{
		vkDestroyImage(device, gBufferImages[i], nullptr);
	}
	
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}

	vkDestroyFramebuffer(device, deferredFrameBuffer, nullptr);

	vkFreeCommandBuffers(device, deferredCommandPool, 1, &deferredCommandBuffer);
	vkFreeCommandBuffers(device, frameBufferCommandPool, static_cast<uint32_t>(frameBufferCommandBuffers.size()), frameBufferCommandBuffers.data());

	for (size_t i = 0; i < materialManager.size(); i++)
	{
		materialManager[i]->cleanPipeline();
	}

	lightingMaterial->cleanPipeline();
	hdrHighlightMaterial->cleanPipeline();
	horizontalMaterial->cleanPipeline();
	verticalMaterial->cleanPipeline();
	lastPostProcessMaterial->cleanPipeline();

	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		debugDisplayMaterials[i]->cleanPipeline();
	}	

	frameBufferMaterial->cleanPipeline();
	
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	
	vkDestroyRenderPass(device, deferredRenderPass, nullptr);


	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];

		vkDestroyRenderPass(device, thisPostProcess->renderPass, nullptr);
	}

	vkDestroyRenderPass(device, frameBufferRenderPass, nullptr);

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanApp::cleanUp()
{
	cleanUpSwapChain();

	AssetDatabase::GetInstance()->cleanUp();

	delete lightingMaterial;
	delete hdrHighlightMaterial;
	delete horizontalMaterial;
	delete verticalMaterial;
	delete lastPostProcessMaterial;
	delete frameBufferMaterial;


	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		delete debugDisplayMaterials[i];
	}

	delete offScreenPlaneforPostProcess;
	delete offScreenPlane;
	delete debugDisplayPlane;

	vkDestroySemaphore(device, objectDrawSemaphore, nullptr);
	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, postProcessSemaphore, nullptr);
	vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

	vkDestroyCommandPool(device, frameBufferCommandPool, nullptr);

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];
		vkDestroyCommandPool(device, thisPostProcess->commandPool, nullptr);
		delete thisPostProcess;
	}

	//vkDestroyCommandPool(device, sceneStage->commandPool, nullptr);
	//delete sceneStage;

	vkDestroyCommandPool(device, deferredCommandPool, nullptr);

	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();

	
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}