#pragma once
// Include the OculusVR SDK
#include <OVR_CAPI_Vk.h>
#include <Extras/OVR_Math.h>
#include "OculusSDKClasses.h"

#include "VulkanDebug.h"
#include "VulkanQueue.h"
#include "VulkanSwapChain.h"

#include "Vertex.h"
#include "PostProcess.h"
#include "../assets/Material.h"
#include "../assets/Geometry.h"
#include "../actors/Object.h"
#include "../actors/Camera.h"
#include "../actors/Light.h"

#include "../core/Shadow.h"

#include "Voxelization.h"

#include <sstream>



static bool leftMouseDown = false;
static bool rightMouseDown = false;
static bool middleMouseDown = false;

static double previousX = 0.0;
static double previousY = 0.0;

static double oldTime = 0.0;
static double currentTime = 0.0;
static int fps = 0;
static int fpstracker = 0;

static std::chrono::time_point<std::chrono::steady_clock> startTime;
static std::chrono::time_point<std::chrono::steady_clock> _oldTime;

static double totalTime;
static double deltaTime;

static float mainLightAngle = 1.0f;

static std::string convertToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

static std::string convertToString(double number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

static Camera camera;

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();





	////////////////////////
	/////// Oculus VR //////
	////////////////////////
	//vars
	ovrSession session;
	ovrFovPort g_EyeFov[2];
	ovrGraphicsLuid luid;
	ovrHmdDesc desc;
	ovrSizei resolution;
	glm::vec3 lastHmdPos;
	glm::vec3 lastHmdEuler;
	OVR::Sizei swapSize;//needed?
	VkExtent2D swapExtent;
	char extensionNames[4096];
	uint32_t extensionNamesSize = sizeof(extensionNames);
	long long frameIndex = 0;
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	TextureSwapChain textureSwapChain;
	RenderPass oculusRenderPass;
	ovrLayerEyeFov layer;
	VkFormat oculusSwapFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat oculusDepthFormat = VK_FORMAT_D32_SFLOAT;
	bool bRenderToHmd = false;
	VkSemaphore mipMapStartSemaphore;
	ovrPosef eyeRenderPose[ovrEye_Count];
	//func
	void initOVR();
	void initOVRLayer();
	void shutdownOVR();
	void queryHmdOrientationAndPosition();







	void initVulkan();
	void initWindow();

	void createInstance();

	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void setupDebugCallback();


	void createSurface();

	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	void createLogicalDevice();
	
	//QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
	void createSwapChain();
	void reCreateSwapChain();
	void cleanUpSwapChain();

	void createImageViews();	
	void createSwapChainImageViews();

	void createFramebuffers();
	void createFrameBufferRenderPass();
	void createFrameBufferCommandPool();
	void createFrameBufferCommandBuffers();

	void createFramebufferDescriptorSetLayout();
	void createFramebufferDescriptorPool();
	void createFramebufferDescriptorSet();


	void createGbuffers();
	void createSceneBuffer();

	void createDeferredFramebuffer();

	void createDeferredRenderPass();
	void createDeferredCommandPool();
	void createDeferredCommandBuffers();

	void drawFrame(float deltaTime);

	void createSemaphores();

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
	void endSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool);
	void transitionMipmapImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange mipSubRange, VkCommandPool commandPool);

	void createDepthResources();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat format);

	void run();
	
	void updateUniformBuffers(unsigned int EYE, float deltaTime);

	void mainLoop();

	void cleanUp();

	void getAsynckeyState();

	VkDevice getDevice()
	{
		return device;
	}
	
	void LoadTexture(std::string path);
	void LoadTextures();

	void LoadObjectMaterials();
	void LoadObjectMaterial(std::string name, std::string albedo, std::string specular, std::string normal, std::string emissive);

	void ConnectSponzaMaterials(Object* sponza);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location,	int32_t code, const char* layerPrefix, const char* msg,	void* userData);
	
	void switchTheLastPostProcess(unsigned int from, unsigned int to)
	{
		if (postProcessStages.size() > to)
		{
			if (theLastPostProcess == postProcessStages[to])
				theLastPostProcess = postProcessStages[from];
			else
				theLastPostProcess = postProcessStages[to];
		}
	}	

	void swingMainLight()
	{
		SwingXAxisDirectionalLight(directionLights[0].lightInfo, 1.0f, mainLightAngle, 0.5f);
	}

	void updateDrawMode()
	{
		void* data;
		vkMapMemory(device, lightingMaterial->optionBufferMemory, 0, sizeof(uint32_t), 0, &data);
		memcpy(data, &drawMode, sizeof(uint32_t));
		vkUnmapMemory(device, lightingMaterial->optionBufferMemory);
	}

	void autoCameraMoving()
	{
		if (autoCameraMove >= 0)
		{
			float speed = 3.0f;
			if (autoCameraMove == 0)
			{
				camera.UpdateOrbit(0.0f, 0.0f, -static_cast<float>(deltaTime)*speed);
			}
			else if (autoCameraMove == 1)
			{
				camera.UpdateOrbit(0.0f, 0.0f, static_cast<float>(deltaTime)*speed);
			}
			else if (autoCameraMove == 2)
			{
				camera.UpdatePosition(-static_cast<float>(deltaTime)*speed, 0.0f, 0.0f);
			}
			else if (autoCameraMove == 3)
			{
				camera.UpdatePosition(static_cast<float>(deltaTime)*speed, 0.0f, 0.0f);
			}
			else if (autoCameraMove == 4)
			{
				camera.UpdateOrbit(static_cast<float>(deltaTime)*speed, 0.0f, 0.0f);
			}
			else if (autoCameraMove == 5)
			{
				camera.UpdateOrbit(-static_cast<float>(deltaTime)*speed, 0.0f, 0.0f);
			}
		}
		

	}

	//bool bIsRightEyeDrawing;

private:	
	GLFWwindow* window;
	GLFWmonitor* primaryMonitor;

	VkInstance instance;
	VkDebugReportCallbackEXT callback;

	//This object will be implicitly destroyed
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue objectDrawQueue;
	
	VkQueue TagQueue;

	VkQueue AllocationQueue;

	VkQueue MipmapQueue;



	VkQueue lightingQueue;

	VkQueue postProcessQueue;

	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;


	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	
	VkRenderPass frameBufferRenderPass;
	VkCommandPool frameBufferCommandPool;
	std::vector<VkCommandBuffer> frameBufferCommandBuffers;

	std::vector<VkCommandBuffer> frameBufferCommandBuffers2;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VoxelConetracingMaterial* voxelConetracingMaterial;
	LightingMaterial* lightingMaterial;

	HDRHighlightMaterial* hdrHighlightMaterial;

	BlurMaterial *HBMaterial;
	BlurMaterial *VBMaterial;

	BlurMaterial *HBMaterial2;
	BlurMaterial *VBMaterial2;

	ComputeBlurMaterial *compHBMaterial;
	ComputeBlurMaterial *compVBMaterial;

	ComputeBlurMaterial *compHBMaterial2;
	ComputeBlurMaterial *compVBMaterial2;

	StandardShadow standardShadow;

	singleTriangular* offScreenPlane;
	singleTriangular* offScreenPlaneforPostProcess;

	singleQuadral* debugDisplayPlane;

	std::vector<DebugDisplayMaterial*> debugDisplayMaterials;

	LastPostProcessgMaterial* lastPostProcessMaterial;

	VoxelRenderMaterial* voxelRenderMaterial;

	//VR BARREL AND ABERRATION
	HDRHighlightMaterial* BarrelAndAberrationPostProcessMaterial;
	FinalRenderingMaterial* frameBufferMaterial;

	VkRenderPass deferredRenderPass;
	VkCommandPool deferredCommandPool;
	VkCommandBuffer deferredCommandBuffer;
	VkFramebuffer deferredFrameBuffer;

	std::vector<VkImage> gBufferImages;
	std::vector<VkDeviceMemory> gBufferImageMemories;
	std::vector<VkImageView>  gBufferImageViews;


	VkImage sceneImage;
	VkDeviceMemory sceneImageMemories;
	VkImageView sceneImageView;

	VkSemaphore objectDrawSemaphore;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore postProcessSemaphore;
	VkSemaphore renderFinishedSemaphore;
	

	uint32_t WIDTH;
	uint32_t HEIGHT;

	uint32_t LayerCount;

	std::vector<DirectionalLight> directionLights;

	std::vector<PostProcess*> postProcessStages;

	Voxelization voxelizator;

	//for FrameRender
	PostProcess* sceneStage;
	PostProcess* theLastPostProcess;

	
	
};

