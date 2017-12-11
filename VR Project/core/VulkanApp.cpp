


#include "VulkanApp.h"

#include "../assets/AssetDatabase.h"

using namespace OVR;
void onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width <= 0 || height <= 0)
		return;
	
	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

	camera.UpdateAspectRatio(static_cast<float>(width) / static_cast<float>(height));
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
	if (leftMouseDown)
	{
		double sensitivity = 20.0;
		
		float deltaX = static_cast<float>((previousX - xPosition) * sensitivity * deltaTime);
		float deltaY = static_cast<float>((previousY - yPosition) * sensitivity* deltaTime);

		camera.UpdateOrbit(deltaX, deltaY, 0.0f);

		previousX = xPosition;
		previousY = yPosition;
	}
	else if (rightMouseDown)
	{
		double sensitivity = 10.0;

		double deltaZ = static_cast<float>((previousY - yPosition) * -sensitivity* deltaTime);

		camera.UpdateOrbit(0.0f, 0.0f, static_cast<float>(deltaZ));

		previousY = yPosition;
	}
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	
	double sensitivity = 2.0;

	float deltaZ = static_cast<float>(yoffset * -sensitivity * deltaTime);

	mainLightAngle += deltaZ;

	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->swingMainLight();
}

void VulkanApp::getAsynckeyState()
{
	double sensitivity = 10.0;

	if (glfwGetKey(window, GLFW_KEY_W) || glfwGetKey(window, GLFW_KEY_UP))
	{
		camera.UpdatePosition(0.0f, 0.0f, static_cast<float>(-sensitivity * deltaTime));
	}

	if (glfwGetKey(window, GLFW_KEY_S) || glfwGetKey(window, GLFW_KEY_DOWN))
	{
		camera.UpdatePosition(0.0f, 0.0f, static_cast<float>(sensitivity* deltaTime));
	}

	if (glfwGetKey(window, GLFW_KEY_A) || glfwGetKey(window, GLFW_KEY_LEFT))
	{
		camera.UpdatePosition(static_cast<float>(-sensitivity* deltaTime), 0.0f, 0.0f);
	}

	if (glfwGetKey(window, GLFW_KEY_D) || glfwGetKey(window, GLFW_KEY_RIGHT))
	{
		camera.UpdatePosition(static_cast<float>(sensitivity* deltaTime), 0.0f, 0.0f);
	}
	queryHmdOrientationAndPosition();
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT || action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_G)
		{
			bDeubDisply = !bDeubDisply;

			VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
			app->reCreateSwapChain();
		}
		
		if (key == GLFW_KEY_V)
		{
			bVRmode = !bVRmode;

			camera.vrMode = bVRmode;

			VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

			app->reCreateSwapChain();
		}

		if (key == GLFW_KEY_R)
		{
			bRotateMainLight = !bRotateMainLight;			
		}

		if (key == GLFW_KEY_I)
		{
			autoCameraMove = autoCameraMove == 0 ? -1 : 0;
		}

		if (key == GLFW_KEY_K)
		{
			autoCameraMove = autoCameraMove == 1 ? -1 : 1;
		}

		if (key == GLFW_KEY_J)
		{
			autoCameraMove = autoCameraMove == 2 ? -1 : 2;
		}

		if (key == GLFW_KEY_L)
		{
			autoCameraMove = autoCameraMove == 3 ? -1 : 3;
		}

		if (key == GLFW_KEY_U)
		{
			autoCameraMove = autoCameraMove == 4 ? -1 : 4;
		}

		if (key == GLFW_KEY_O)
		{
			autoCameraMove = autoCameraMove == 5 ? -1 : 5;
		}


		if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5)
		{
			drawMode = key - GLFW_KEY_1;
			VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
			app->updateDrawMode();
		}

		if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
		{
			VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
			app->switchTheLastPostProcess(7, key - GLFW_KEY_F1);
			app->reCreateSwapChain();
			
		}

	}

	
}

//OCULUS res: 2160 1200, halfres: 1080 600
VulkanApp::VulkanApp():WIDTH(1080), HEIGHT(600), physicalDevice(VK_NULL_HANDLE), LayerCount(1)
{

	lightingMaterial = NULL;
	hdrHighlightMaterial = NULL;

	HBMaterial = NULL;
	VBMaterial = NULL;;
	HBMaterial2 = NULL;;
	VBMaterial2 = NULL;

	compHBMaterial = NULL;
	compVBMaterial = NULL;
	compHBMaterial2 = NULL;
	compVBMaterial2 = NULL;

	lastPostProcessMaterial = NULL;

	
	voxelRenderMaterial = NULL;

	voxelConetracingMaterial = NULL;
	BarrelAndAberrationPostProcessMaterial = NULL;

	frameBufferMaterial = NULL;

}

VulkanApp::~VulkanApp()
{
}

void VulkanApp::initWindow()
{
	glfwInit();

	primaryMonitor = glfwGetPrimaryMonitor();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);


	//window = glfwCreateWindow(1920, 1080, "VR Project", primaryMonitor, nullptr);
	window = glfwCreateWindow(WIDTH, HEIGHT, "VR Project", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, onWindowResized);
	glfwSetMouseButtonCallback(window, mouseDownCallback);
	glfwSetCursorPosCallback(window, mouseMoveCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetKeyCallback(window, keyboardCallback);
}

void VulkanApp::LoadTexture(std::string path)
{
	AssetDatabase::GetInstance()->LoadAsset<Texture>(path);
	AssetDatabase::GetInstance()->textureList.push_back(path);
}

void VulkanApp::LoadTextures()
{
	LoadTexture("textures/storm_hero_d3crusaderf_base_diff.tga");
	LoadTexture("textures/storm_hero_d3crusaderf_base_spec.tga");
	LoadTexture("textures/storm_hero_d3crusaderf_base_norm.tga");
	LoadTexture("textures/storm_hero_d3crusaderf_base_emis.tga");

	LoadTexture("textures/storm_hero_chromie_ultimate_diff.tga");
	LoadTexture("textures/storm_hero_chromie_ultimate_spec.tga");
	LoadTexture("textures/storm_hero_chromie_ultimate_norm.tga");
	LoadTexture("textures/storm_hero_chromie_ultimate_emis.tga");

	LoadTexture("textures/Cerberus/Cerberus_A.tga");
	LoadTexture("textures/Cerberus/Cerberus_S.tga");
	LoadTexture("textures/Cerberus/Cerberus_N.tga");
	LoadTexture("textures/Cerberus/Cerberus_E.tga");

	LoadTexture("textures/sponza/no_emis.tga");

	//arch
	LoadTexture("textures/sponza/arch/arch_albedo.tga");
	LoadTexture("textures/sponza/arch/arch_spec.tga");
	LoadTexture("textures/sponza/arch/arch_norm.tga");
	
	//bricks
	LoadTexture("textures/sponza/bricks/bricks_albedo.tga");
	LoadTexture("textures/sponza/bricks/bricks_spec.tga");
	LoadTexture("textures/sponza/bricks/bricks_norm.tga");

	//celing
	LoadTexture("textures/sponza/ceiling/ceiling_albedo.tga");
	LoadTexture("textures/sponza/ceiling/ceiling_spec.tga");
	LoadTexture("textures/sponza/ceiling/ceiling_norm.tga");

	//column
	LoadTexture("textures/sponza/column/column_a_albedo.tga");
	LoadTexture("textures/sponza/column/column_a_spec.tga");
	LoadTexture("textures/sponza/column/column_a_norm.tga");
	LoadTexture("textures/sponza/column/column_b_albedo.tga");
	LoadTexture("textures/sponza/column/column_b_spec.tga");
	LoadTexture("textures/sponza/column/column_b_norm.tga");
	LoadTexture("textures/sponza/column/column_c_albedo.tga");
	LoadTexture("textures/sponza/column/column_c_spec.tga");
	LoadTexture("textures/sponza/column/column_c_norm.tga");

	//curtain
	LoadTexture("textures/sponza/curtain/sponza_curtain_blue_albedo.tga");
	LoadTexture("textures/sponza/curtain/sponza_curtain_green_albedo.tga");
	LoadTexture("textures/sponza/curtain/sponza_curtain_red_albedo.tga");

	LoadTexture("textures/sponza/curtain/sponza_curtain_blue_spec.tga");
	LoadTexture("textures/sponza/curtain/sponza_curtain_green_spec.tga");
	LoadTexture("textures/sponza/curtain/sponza_curtain_red_spec.tga");

	LoadTexture("textures/sponza/curtain/sponza_curtain_norm.tga");

	//detail
	LoadTexture("textures/sponza/detail/detail_albedo.tga");
	LoadTexture("textures/sponza/detail/detail_spec.tga");
	LoadTexture("textures/sponza/detail/detail_norm.tga");

	//fabric
	LoadTexture("textures/sponza/fabric/fabric_blue_albedo.tga");
	LoadTexture("textures/sponza/fabric/fabric_blue_spec.tga");
	LoadTexture("textures/sponza/fabric/fabric_green_albedo.tga");

	LoadTexture("textures/sponza/fabric/fabric_green_spec.tga");
	LoadTexture("textures/sponza/fabric/fabric_red_albedo.tga");
	LoadTexture("textures/sponza/fabric/fabric_red_spec.tga");

	LoadTexture("textures/sponza/fabric/fabric_norm.tga");

	//flagpole
	LoadTexture("textures/sponza/flagpole/flagpole_albedo.tga");
	LoadTexture("textures/sponza/flagpole/flagpole_spec.tga");
	LoadTexture("textures/sponza/flagpole/flagpole_norm.tga");

	//floor
	LoadTexture("textures/sponza/floor/floor_albedo.tga");
	LoadTexture("textures/sponza/floor/floor_spec.tga");
	LoadTexture("textures/sponza/floor/floor_norm.tga");

	//lion
	LoadTexture("textures/sponza/lion/lion_albedo.tga");
	LoadTexture("textures/sponza/lion/lion_norm.tga");
	LoadTexture("textures/sponza/lion/lion_spec.tga");

	//lion_back
	LoadTexture("textures/sponza/lion_background/lion_background_albedo.tga");
	LoadTexture("textures/sponza/lion_background/lion_background_spec.tga");
	LoadTexture("textures/sponza/lion_background/lion_background_norm.tga");

	//plant
	LoadTexture("textures/sponza/plant/vase_plant_albedo.tga");
	LoadTexture("textures/sponza/plant/vase_plant_spec.tga");
	LoadTexture("textures/sponza/plant/vase_plant_norm.tga");
	LoadTexture("textures/sponza/plant/vase_plant_emiss.tga");

	//roof
	LoadTexture("textures/sponza/roof/roof_albedo.tga");
	LoadTexture("textures/sponza/roof/roof_spec.tga");
	LoadTexture("textures/sponza/roof/roof_norm.tga");

	//thorn
	LoadTexture("textures/sponza/thorn/sponza_thorn_albedo.tga");
	LoadTexture("textures/sponza/thorn/sponza_thorn_spec.tga");
	LoadTexture("textures/sponza/thorn/sponza_thorn_norm.tga");
	LoadTexture("textures/sponza/thorn/sponza_thorn_emis.tga");

	//vase
	LoadTexture("textures/sponza/vase/vase_albedo.tga");
	LoadTexture("textures/sponza/vase/vase_spec.tga");
	LoadTexture("textures/sponza/vase/vase_norm.tga");

	//vase others
	LoadTexture("textures/sponza/vase_hanging/vase_hanging_albedo.tga");
	LoadTexture("textures/sponza/vase_hanging/vase_round_albedo.tga");
	LoadTexture("textures/sponza/vase_hanging/vase_round_spec.tga");
	LoadTexture("textures/sponza/vase_hanging/vase_round_norm.tga");

	//chain
	LoadTexture("textures/sponza/chain/chain_albedo.tga");
	LoadTexture("textures/sponza/chain/chain_spec.tga");
	LoadTexture("textures/sponza/chain/chain_norm.tga");
	

}

void VulkanApp::LoadObjectMaterials()
{
	LoadObjectMaterial("standard_material", "textures/storm_hero_d3crusaderf_base_diff.tga", "textures/storm_hero_d3crusaderf_base_spec.tga", "textures/storm_hero_d3crusaderf_base_norm.tga", "textures/storm_hero_d3crusaderf_base_emis.tga");
	LoadObjectMaterial("standard_material2", "textures/storm_hero_chromie_ultimate_diff.tga", "textures/storm_hero_chromie_ultimate_spec.tga", "textures/storm_hero_chromie_ultimate_norm.tga", "textures/storm_hero_chromie_ultimate_emis.tga");
	LoadObjectMaterial("standard_material3", "textures/Cerberus/Cerberus_A.tga", "textures/Cerberus/Cerberus_S.tga", "textures/Cerberus/Cerberus_N.tga", "textures/Cerberus/Cerberus_E.tga");

	//arch
	LoadObjectMaterial("arch", "textures/sponza/arch/arch_albedo.tga", "textures/sponza/arch/arch_spec.tga", "textures/sponza/arch/arch_norm.tga", "textures/sponza/no_emis.tga");

	//bricks
	LoadObjectMaterial("bricks", "textures/sponza/bricks/bricks_albedo.tga", "textures/sponza/bricks/bricks_spec.tga", "textures/sponza/bricks/bricks_norm.tga", "textures/sponza/no_emis.tga");

	//ceiling
	LoadObjectMaterial("ceiling", "textures/sponza/ceiling/ceiling_albedo.tga", "textures/sponza/ceiling/ceiling_spec.tga", "textures/sponza/ceiling/ceiling_norm.tga", "textures/sponza/no_emis.tga");

	//chain
	LoadObjectMaterial("chain", "textures/sponza/chain/chain_albedo.tga", "textures/sponza/chain/chain_spec.tga", "textures/sponza/chain/chain_norm.tga", "textures/sponza/no_emis.tga");

	//column_a
	LoadObjectMaterial("column_a", "textures/sponza/column/column_a_albedo.tga", "textures/sponza/column/column_a_spec.tga", "textures/sponza/column/column_a_norm.tga", "textures/sponza/no_emis.tga");
	//column_b
	LoadObjectMaterial("column_b", "textures/sponza/column/column_b_albedo.tga", "textures/sponza/column/column_b_spec.tga", "textures/sponza/column/column_b_norm.tga", "textures/sponza/no_emis.tga");
	//column_c
	LoadObjectMaterial("column_c", "textures/sponza/column/column_c_albedo.tga", "textures/sponza/column/column_c_spec.tga", "textures/sponza/column/column_c_norm.tga", "textures/sponza/no_emis.tga");


	//curtain_blue
	LoadObjectMaterial("curtain_blue", "textures/sponza/curtain/sponza_curtain_blue_albedo.tga", "textures/sponza/curtain/sponza_curtain_blue_spec.tga", "textures/sponza/curtain/sponza_curtain_norm.tga", "textures/sponza/no_emis.tga");

	//curtain_green
	LoadObjectMaterial("curtain_green", "textures/sponza/curtain/sponza_curtain_green_albedo.tga", "textures/sponza/curtain/sponza_curtain_green_spec.tga", "textures/sponza/curtain/sponza_curtain_norm.tga", "textures/sponza/no_emis.tga");

	//curtain_red
	LoadObjectMaterial("curtain_red", "textures/sponza/curtain/sponza_curtain_red_albedo.tga", "textures/sponza/curtain/sponza_curtain_red_spec.tga", "textures/sponza/curtain/sponza_curtain_norm.tga", "textures/sponza/no_emis.tga");
		
	//detail
	LoadObjectMaterial("detail", "textures/sponza/detail/detail_albedo.tga", "textures/sponza/detail/detail_spec.tga", "textures/sponza/detail/detail_norm.tga", "textures/sponza/no_emis.tga");
		
	//fabric_blue
	LoadObjectMaterial("fabric_blue", "textures/sponza/fabric/fabric_blue_albedo.tga", "textures/sponza/fabric/fabric_blue_spec.tga", "textures/sponza/fabric/fabric_norm.tga", "textures/sponza/no_emis.tga");

	//fabric_green
	LoadObjectMaterial("fabric_green", "textures/sponza/fabric/fabric_green_albedo.tga", "textures/sponza/fabric/fabric_green_spec.tga", "textures/sponza/fabric/fabric_norm.tga", "textures/sponza/no_emis.tga");

	//fabric_red
	LoadObjectMaterial("fabric_red", "textures/sponza/fabric/fabric_red_albedo.tga", "textures/sponza/fabric/fabric_red_spec.tga", "textures/sponza/fabric/fabric_norm.tga", "textures/sponza/no_emis.tga");

	//flagpole
	LoadObjectMaterial("flagpole", "textures/sponza/flagpole/flagpole_albedo.tga", "textures/sponza/flagpole/flagpole_spec.tga", "textures/sponza/flagpole/flagpole_norm.tga", "textures/sponza/no_emis.tga");

	//floor
	LoadObjectMaterial("floor", "textures/sponza/floor/floor_albedo.tga", "textures/sponza/floor/floor_spec.tga", "textures/sponza/floor/floor_norm.tga", "textures/sponza/no_emis.tga");

	//lion
	LoadObjectMaterial("lion", "textures/sponza/lion/lion_albedo.tga", "textures/sponza/lion/lion_spec.tga", "textures/sponza/lion/lion_norm.tga", "textures/sponza/no_emis.tga");
	
	//lion_back
	LoadObjectMaterial("lion_back", "textures/sponza/lion_background/lion_background_albedo.tga", "textures/sponza/lion_background/lion_background_spec.tga", "textures/sponza/lion_background/lion_background_norm.tga", "textures/sponza/no_emis.tga");

	//plant
	LoadObjectMaterial("plant", "textures/sponza/plant/vase_plant_albedo.tga", "textures/sponza/plant/vase_plant_spec.tga", "textures/sponza/plant/vase_plant_norm.tga", "textures/sponza/plant/vase_plant_emiss.tga");

	//roof
	LoadObjectMaterial("roof", "textures/sponza/roof/roof_albedo.tga", "textures/sponza/roof/roof_spec.tga", "textures/sponza/roof/roof_norm.tga", "textures/sponza/no_emis.tga");

	//thorn
	LoadObjectMaterial("thorn", "textures/sponza/thorn/sponza_thorn_albedo.tga", "textures/sponza/thorn/sponza_thorn_spec.tga", "textures/sponza/thorn/sponza_thorn_norm.tga", "textures/sponza/thorn/sponza_thorn_emis.tga");

	//vase
	LoadObjectMaterial("vase", "textures/sponza/vase/vase_albedo.tga", "textures/sponza/vase/vase_spec.tga", "textures/sponza/vase/vase_norm.tga", "textures/sponza/no_emis.tga");

	//vase_hanging
	LoadObjectMaterial("vase_hanging", "textures/sponza/vase_hanging/vase_hanging_albedo.tga", "textures/sponza/vase_hanging/vase_round_spec.tga", "textures/sponza/vase_hanging/vase_round_norm.tga", "textures/sponza/no_emis.tga");

	//vase_round
	LoadObjectMaterial("vase_round", "textures/sponza/vase_hanging/vase_round_albedo.tga", "textures/sponza/vase_hanging/vase_round_spec.tga", "textures/sponza/vase_hanging/vase_round_norm.tga", "textures/sponza/no_emis.tga");

}

void VulkanApp::LoadObjectMaterial(std::string name, std::string albedo, std::string specular, std::string normal, std::string emissive)
{
	ObjectDrawMaterial* tempMat = new ObjectDrawMaterial;
	tempMat->LoadFromFilename(device, physicalDevice, deferredCommandPool, objectDrawQueue, name);
	tempMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>(albedo));
	tempMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>(specular));
	tempMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>(normal));
	tempMat->addTexture(AssetDatabase::GetInstance()->LoadAsset<Texture>(emissive));
	tempMat->setShaderPaths("shaders/shader.vert.spv", "shaders/shader.frag.spv", "", "", "", "");
	tempMat->createDescriptorSet();

	materialManager.push_back(tempMat);
}

void VulkanApp::ConnectSponzaMaterials(Object* sponza)
{
	Material* thorn = AssetDatabase::LoadMaterial("thorn");
	sponza->connectMaterial(thorn, 0); //Leaf
	sponza->connectMaterial(thorn, 275); //Leaf
	sponza->connectMaterial(thorn, 276); //Leaf
	sponza->connectMaterial(thorn, 277); //Leaf
	sponza->connectMaterial(thorn, 278); //Leaf
	sponza->connectMaterial(thorn, 279); //Leaf
	sponza->connectMaterial(thorn, 280); //Leaf
	sponza->connectMaterial(thorn, 281); //Leaf

	Material* fabric_e = AssetDatabase::LoadMaterial("fabric_green");

	sponza->connectMaterial(fabric_e, 282); //curtain_red
	sponza->connectMaterial(fabric_e, 285); //curtain_red
	sponza->connectMaterial(fabric_e, 287); //curtain_red

	Material* fabric_g = AssetDatabase::LoadMaterial("curtain_blue");
	sponza->connectMaterial(fabric_g, 320); //curtain_green
	sponza->connectMaterial(fabric_g, 326); //curtain_green
	sponza->connectMaterial(fabric_g, 329); //curtain_green

	Material* fabric_c = AssetDatabase::LoadMaterial("curtain_red");
	sponza->connectMaterial(fabric_c, 321); //curtain_red
	sponza->connectMaterial(fabric_c, 323); //curtain_red
	sponza->connectMaterial(fabric_c, 325); //curtain_red
	sponza->connectMaterial(fabric_c, 328); //curtain_red

	Material* fabric_f = AssetDatabase::LoadMaterial("curtain_green");
	sponza->connectMaterial(fabric_f, 322); //curtain_blue
	sponza->connectMaterial(fabric_f, 324); //curtain_blue
	sponza->connectMaterial(fabric_f, 327); //curtain_blue

	Material* fabric_a = AssetDatabase::LoadMaterial("fabric_red");
	sponza->connectMaterial(fabric_a, 283); //fabric_red
	sponza->connectMaterial(fabric_a, 286); //fabric_red
	sponza->connectMaterial(fabric_a, 289); //fabric_red

	Material* fabric_d = AssetDatabase::LoadMaterial("fabric_blue");
	sponza->connectMaterial(fabric_d, 284); //fabric_blue
	sponza->connectMaterial(fabric_d, 288); //fabric_blue

	Material* chain = AssetDatabase::LoadMaterial("chain");
	sponza->connectMaterial(chain, 330); //chain
	sponza->connectMaterial(chain, 331); //chain
	sponza->connectMaterial(chain, 332); //chain
	sponza->connectMaterial(chain, 333); //chain

	sponza->connectMaterial(chain, 339); //chain
	sponza->connectMaterial(chain, 340); //chain
	sponza->connectMaterial(chain, 341); //chain
	sponza->connectMaterial(chain, 342); //chain

	sponza->connectMaterial(chain, 348); //chain
	sponza->connectMaterial(chain, 349); //chain
	sponza->connectMaterial(chain, 350); //chain
	sponza->connectMaterial(chain, 351); //chain

	sponza->connectMaterial(chain, 357); //chain
	sponza->connectMaterial(chain, 358); //chain
	sponza->connectMaterial(chain, 359); //chain
	sponza->connectMaterial(chain, 360); //chain

	Material* vase_hanging = AssetDatabase::LoadMaterial("vase_hanging");
	sponza->connectMaterial(vase_hanging, 334); //vase_hanging
	sponza->connectMaterial(vase_hanging, 335); //vase_hanging
	sponza->connectMaterial(vase_hanging, 336); //vase_hanging
	sponza->connectMaterial(vase_hanging, 337); //vase_hanging
	sponza->connectMaterial(vase_hanging, 338); //vase_hanging
	sponza->connectMaterial(vase_hanging, 343); //vase_hanging
	sponza->connectMaterial(vase_hanging, 344); //vase_hanging
	sponza->connectMaterial(vase_hanging, 345); //vase_hanging
	sponza->connectMaterial(vase_hanging, 346); //vase_hanging
	sponza->connectMaterial(vase_hanging, 347); //vase_hanging
	sponza->connectMaterial(vase_hanging, 352); //vase_hanging
	sponza->connectMaterial(vase_hanging, 353); //vase_hanging
	sponza->connectMaterial(vase_hanging, 354); //vase_hanging
	sponza->connectMaterial(vase_hanging, 355); //vase_hanging
	sponza->connectMaterial(vase_hanging, 356); //vase_hanging
	sponza->connectMaterial(vase_hanging, 361); //vase_hanging
	sponza->connectMaterial(vase_hanging, 362); //vase_hanging
	sponza->connectMaterial(vase_hanging, 363); //vase_hanging
	sponza->connectMaterial(vase_hanging, 364); //vase_hanging
	sponza->connectMaterial(vase_hanging, 365); //vase_hanging



	sponza->connectMaterial(AssetDatabase::LoadMaterial("thorn"), 1); //Material__57

	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 366); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 367); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 368); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 369); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 370); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 371); //Material__57
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 372); //Material__57


	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase"), 373); //vase
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase"), 374); //vase
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase"), 375); //vase
	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase"), 376); //vase

	sponza->connectMaterial(AssetDatabase::LoadMaterial("vase_round"), 2); //vase_round

	sponza->connectMaterial(AssetDatabase::LoadMaterial("lion_back"), 3); //Material__298

	sponza->connectMaterial(AssetDatabase::LoadMaterial("lion"), 377); //Material__25
	sponza->connectMaterial(AssetDatabase::LoadMaterial("lion"), 378); //Material__25

	sponza->connectMaterial(AssetDatabase::LoadMaterial("fabric_red"), 4); //16___Default

	Material* bricks = AssetDatabase::LoadMaterial("bricks");
	sponza->connectMaterial(bricks, 5); //bricks
	sponza->connectMaterial(bricks, 6); //bricks
	sponza->connectMaterial(bricks, 34); //bricks
	sponza->connectMaterial(bricks, 36); //bricks
	sponza->connectMaterial(bricks, 66); //bricks
	sponza->connectMaterial(bricks, 68); //bricks
	sponza->connectMaterial(bricks, 69); //bricks
	sponza->connectMaterial(bricks, 75); //bricks
	sponza->connectMaterial(bricks, 116); //bricks
	sponza->connectMaterial(bricks, 258); //bricks
	sponza->connectMaterial(bricks, 379); //bricks
	sponza->connectMaterial(bricks, 382); //bricks

	Material* roof = AssetDatabase::LoadMaterial("roof");
	sponza->connectMaterial(roof, 380); //roof
	sponza->connectMaterial(roof, 381); //roof


	Material* arch = AssetDatabase::LoadMaterial("arch");
	sponza->connectMaterial(arch, 7); //arch
	sponza->connectMaterial(arch, 17); //arch
	sponza->connectMaterial(arch, 20); //arch
	sponza->connectMaterial(arch, 21); //arch
	sponza->connectMaterial(arch, 37); //arch
	sponza->connectMaterial(arch, 39); //arch
	sponza->connectMaterial(arch, 41); //arch
	sponza->connectMaterial(arch, 43); //arch
	sponza->connectMaterial(arch, 45); //arch
	sponza->connectMaterial(arch, 47); //arch
	sponza->connectMaterial(arch, 49); //arch
	sponza->connectMaterial(arch, 51); //arch
	sponza->connectMaterial(arch, 53); //arch
	sponza->connectMaterial(arch, 55); //arch
	sponza->connectMaterial(arch, 56); //arch
	sponza->connectMaterial(arch, 57); //arch
	sponza->connectMaterial(arch, 58); //arch
	sponza->connectMaterial(arch, 59); //arch
	sponza->connectMaterial(arch, 60); //arch
	sponza->connectMaterial(arch, 61); //arch
	sponza->connectMaterial(arch, 62); //arch
	sponza->connectMaterial(arch, 63); //arch
	sponza->connectMaterial(arch, 64); //arch
	sponza->connectMaterial(arch, 65); //arch
	sponza->connectMaterial(arch, 67); //arch
	sponza->connectMaterial(arch, 122); //arch
	sponza->connectMaterial(arch, 123); //arch
	sponza->connectMaterial(arch, 124); //arch

	Material* ceiling = AssetDatabase::LoadMaterial("ceiling");
	sponza->connectMaterial(ceiling, 8); //ceiling
	sponza->connectMaterial(ceiling, 19); //ceiling
	sponza->connectMaterial(ceiling, 35); //ceiling
	sponza->connectMaterial(ceiling, 38); //ceiling
	sponza->connectMaterial(ceiling, 40); //ceiling
	sponza->connectMaterial(ceiling, 42); //ceiling
	sponza->connectMaterial(ceiling, 44); //ceiling
	sponza->connectMaterial(ceiling, 46); //ceiling
	sponza->connectMaterial(ceiling, 48); //ceiling
	sponza->connectMaterial(ceiling, 50); //ceiling
	sponza->connectMaterial(ceiling, 52); //ceiling
	sponza->connectMaterial(ceiling, 54); //ceiling

	Material* column_a = AssetDatabase::LoadMaterial("column_a");
	sponza->connectMaterial(column_a, 9); //column_a
	sponza->connectMaterial(column_a, 10); //column_a
	sponza->connectMaterial(column_a, 11); //column_a
	sponza->connectMaterial(column_a, 12); //column_a
	sponza->connectMaterial(column_a, 13); //column_a
	sponza->connectMaterial(column_a, 14); //column_a
	sponza->connectMaterial(column_a, 15); //column_a
	sponza->connectMaterial(column_a, 16); //column_a
	sponza->connectMaterial(column_a, 118); //column_a
	sponza->connectMaterial(column_a, 119); //column_a
	sponza->connectMaterial(column_a, 120); //column_a
	sponza->connectMaterial(column_a, 121); //column_a

	Material* column_b = AssetDatabase::LoadMaterial("column_b");
	sponza->connectMaterial(column_b, 125); //column_b
	sponza->connectMaterial(column_b, 126); //column_b
	sponza->connectMaterial(column_b, 127); //column_b
	sponza->connectMaterial(column_b, 128); //column_b
	sponza->connectMaterial(column_b, 129); //column_b
	sponza->connectMaterial(column_b, 130); //column_b
	sponza->connectMaterial(column_b, 131); //column_b
	sponza->connectMaterial(column_b, 132); //column_b
	sponza->connectMaterial(column_b, 133); //column_b
	sponza->connectMaterial(column_b, 134); //column_b
	sponza->connectMaterial(column_b, 135); //column_b
	sponza->connectMaterial(column_b, 136); //column_b
	sponza->connectMaterial(column_b, 137); //column_b
	sponza->connectMaterial(column_b, 138); //column_b
	sponza->connectMaterial(column_b, 139); //column_b
	sponza->connectMaterial(column_b, 140); //column_b
	sponza->connectMaterial(column_b, 141); //column_b
	sponza->connectMaterial(column_b, 142); //column_b
	sponza->connectMaterial(column_b, 143); //column_b
	sponza->connectMaterial(column_b, 144); //column_b
	sponza->connectMaterial(column_b, 145); //column_b
	sponza->connectMaterial(column_b, 146); //column_b
	sponza->connectMaterial(column_b, 147); //column_b
	sponza->connectMaterial(column_b, 148); //column_b
	sponza->connectMaterial(column_b, 149); //column_b
	sponza->connectMaterial(column_b, 150); //column_b
	sponza->connectMaterial(column_b, 151); //column_b
	sponza->connectMaterial(column_b, 152); //column_b
	sponza->connectMaterial(column_b, 153); //column_b
	sponza->connectMaterial(column_b, 154); //column_b
	sponza->connectMaterial(column_b, 155); //column_b
	sponza->connectMaterial(column_b, 156); //column_b
	sponza->connectMaterial(column_b, 157); //column_b
	sponza->connectMaterial(column_b, 158); //column_b
	sponza->connectMaterial(column_b, 159); //column_b
	sponza->connectMaterial(column_b, 160); //column_b
	sponza->connectMaterial(column_b, 161); //column_b
	sponza->connectMaterial(column_b, 162); //column_b
	sponza->connectMaterial(column_b, 163); //column_b
	sponza->connectMaterial(column_b, 164); //column_b
	sponza->connectMaterial(column_b, 165); //column_b
	sponza->connectMaterial(column_b, 166); //column_b
	sponza->connectMaterial(column_b, 167); //column_b
	sponza->connectMaterial(column_b, 168); //column_b
	sponza->connectMaterial(column_b, 169); //column_b
	sponza->connectMaterial(column_b, 170); //column_b
	sponza->connectMaterial(column_b, 171); //column_b
	sponza->connectMaterial(column_b, 172); //column_b
	sponza->connectMaterial(column_b, 173); //column_b
	sponza->connectMaterial(column_b, 174); //column_b
	sponza->connectMaterial(column_b, 175); //column_b
	sponza->connectMaterial(column_b, 176); //column_b
	sponza->connectMaterial(column_b, 177); //column_b
	sponza->connectMaterial(column_b, 178); //column_b
	sponza->connectMaterial(column_b, 179); //column_b
	sponza->connectMaterial(column_b, 180); //column_b
	sponza->connectMaterial(column_b, 181); //column_b
	sponza->connectMaterial(column_b, 182); //column_b
	sponza->connectMaterial(column_b, 183); //column_b
	sponza->connectMaterial(column_b, 184); //column_b
	sponza->connectMaterial(column_b, 185); //column_b
	sponza->connectMaterial(column_b, 186); //column_b
	sponza->connectMaterial(column_b, 187); //column_b
	sponza->connectMaterial(column_b, 188); //column_b
	sponza->connectMaterial(column_b, 189); //column_b
	sponza->connectMaterial(column_b, 190); //column_b
	sponza->connectMaterial(column_b, 191); //column_b
	sponza->connectMaterial(column_b, 192); //column_b
	sponza->connectMaterial(column_b, 193); //column_b
	sponza->connectMaterial(column_b, 194); //column_b
	sponza->connectMaterial(column_b, 195); //column_b
	sponza->connectMaterial(column_b, 196); //column_b
	sponza->connectMaterial(column_b, 197); //column_b
	sponza->connectMaterial(column_b, 198); //column_b
	sponza->connectMaterial(column_b, 199); //column_b
	sponza->connectMaterial(column_b, 200); //column_b
	sponza->connectMaterial(column_b, 201); //column_b
	sponza->connectMaterial(column_b, 202); //column_b
	sponza->connectMaterial(column_b, 203); //column_b
	sponza->connectMaterial(column_b, 204); //column_b
	sponza->connectMaterial(column_b, 205); //column_b
	sponza->connectMaterial(column_b, 206); //column_b
	sponza->connectMaterial(column_b, 207); //column_b
	sponza->connectMaterial(column_b, 208); //column_b
	sponza->connectMaterial(column_b, 209); //column_b
	sponza->connectMaterial(column_b, 210); //column_b
	sponza->connectMaterial(column_b, 211); //column_b
	sponza->connectMaterial(column_b, 212); //column_b
	sponza->connectMaterial(column_b, 213); //column_b
	sponza->connectMaterial(column_b, 214); //column_b
	sponza->connectMaterial(column_b, 215); //column_b
	sponza->connectMaterial(column_b, 216); //column_b
	sponza->connectMaterial(column_b, 217); //column_b
	sponza->connectMaterial(column_b, 218); //column_b
	sponza->connectMaterial(column_b, 219); //column_b
	sponza->connectMaterial(column_b, 220); //column_b
	sponza->connectMaterial(column_b, 221); //column_b
	sponza->connectMaterial(column_b, 222); //column_b
	sponza->connectMaterial(column_b, 223); //column_b
	sponza->connectMaterial(column_b, 224); //column_b
	sponza->connectMaterial(column_b, 225); //column_b
	sponza->connectMaterial(column_b, 226); //column_b
	sponza->connectMaterial(column_b, 227); //column_b
	sponza->connectMaterial(column_b, 228); //column_b
	sponza->connectMaterial(column_b, 229); //column_b
	sponza->connectMaterial(column_b, 230); //column_b
	sponza->connectMaterial(column_b, 231); //column_b
	sponza->connectMaterial(column_b, 232); //column_b
	sponza->connectMaterial(column_b, 233); //column_b
	sponza->connectMaterial(column_b, 234); //column_b
	sponza->connectMaterial(column_b, 235); //column_b
	sponza->connectMaterial(column_b, 236); //column_b
	sponza->connectMaterial(column_b, 237); //column_b
	sponza->connectMaterial(column_b, 238); //column_b
	sponza->connectMaterial(column_b, 239); //column_b
	sponza->connectMaterial(column_b, 240); //column_b
	sponza->connectMaterial(column_b, 241); //column_b
	sponza->connectMaterial(column_b, 242); //column_b
	sponza->connectMaterial(column_b, 243); //column_b
	sponza->connectMaterial(column_b, 244); //column_b
	sponza->connectMaterial(column_b, 245); //column_b
	sponza->connectMaterial(column_b, 246); //column_b
	sponza->connectMaterial(column_b, 247); //column_b
	sponza->connectMaterial(column_b, 248); //column_b
	sponza->connectMaterial(column_b, 249); //column_b
	sponza->connectMaterial(column_b, 250); //column_b
	sponza->connectMaterial(column_b, 251); //column_b
	sponza->connectMaterial(column_b, 252); //column_b
	sponza->connectMaterial(column_b, 253); //column_b
	sponza->connectMaterial(column_b, 254); //column_b
	sponza->connectMaterial(column_b, 255); //column_b
	sponza->connectMaterial(column_b, 256); //column_b
	sponza->connectMaterial(column_b, 257); //column_b

	Material* column_c = AssetDatabase::LoadMaterial("column_c");
	sponza->connectMaterial(column_c, 22); //column_c
	sponza->connectMaterial(column_c, 23); //column_c
	sponza->connectMaterial(column_c, 24); //column_c
	sponza->connectMaterial(column_c, 25); //column_c
	sponza->connectMaterial(column_c, 26); //column_c
	sponza->connectMaterial(column_c, 27); //column_c
	sponza->connectMaterial(column_c, 28); //column_c
	sponza->connectMaterial(column_c, 29); //column_c
	sponza->connectMaterial(column_c, 30); //column_c
	sponza->connectMaterial(column_c, 31); //column_c
	sponza->connectMaterial(column_c, 32); //column_c
	sponza->connectMaterial(column_c, 33); //column_c
	sponza->connectMaterial(column_c, 76); //column_c
	sponza->connectMaterial(column_c, 77); //column_c
	sponza->connectMaterial(column_c, 78); //column_c
	sponza->connectMaterial(column_c, 79); //column_c
	sponza->connectMaterial(column_c, 80); //column_c
	sponza->connectMaterial(column_c, 81); //column_c
	sponza->connectMaterial(column_c, 82); //column_c
	sponza->connectMaterial(column_c, 83); //column_c
	sponza->connectMaterial(column_c, 84); //column_c
	sponza->connectMaterial(column_c, 85); //column_c
	sponza->connectMaterial(column_c, 86); //column_c
	sponza->connectMaterial(column_c, 87); //column_c
	sponza->connectMaterial(column_c, 88); //column_c
	sponza->connectMaterial(column_c, 89); //column_c
	sponza->connectMaterial(column_c, 90); //column_c
	sponza->connectMaterial(column_c, 91); //column_c
	sponza->connectMaterial(column_c, 92); //column_c
	sponza->connectMaterial(column_c, 93); //column_c
	sponza->connectMaterial(column_c, 94); //column_c
	sponza->connectMaterial(column_c, 95); //column_c
	sponza->connectMaterial(column_c, 96); //column_c
	sponza->connectMaterial(column_c, 97); //column_c
	sponza->connectMaterial(column_c, 98); //column_c
	sponza->connectMaterial(column_c, 99); //column_c
	sponza->connectMaterial(column_c, 100); //column_c
	sponza->connectMaterial(column_c, 101); //column_c
	sponza->connectMaterial(column_c, 102); //column_c
	sponza->connectMaterial(column_c, 103); //column_c
	sponza->connectMaterial(column_c, 104); //column_c
	sponza->connectMaterial(column_c, 105); //column_c
	sponza->connectMaterial(column_c, 106); //column_c
	sponza->connectMaterial(column_c, 107); //column_c
	sponza->connectMaterial(column_c, 108); //column_c
	sponza->connectMaterial(column_c, 109); //column_c
	sponza->connectMaterial(column_c, 110); //column_c
	sponza->connectMaterial(column_c, 111); //column_c
	sponza->connectMaterial(column_c, 112); //column_c
	sponza->connectMaterial(column_c, 113); //column_c
	sponza->connectMaterial(column_c, 114); //column_c
	sponza->connectMaterial(column_c, 115); //column_c

	Material* floor = AssetDatabase::LoadMaterial("floor");
	sponza->connectMaterial(floor, 18); //floor
	sponza->connectMaterial(floor, 117); //floor


	Material* detail = AssetDatabase::LoadMaterial("detail");
	sponza->connectMaterial(detail, 70); //detail
	sponza->connectMaterial(detail, 71); //detail
	sponza->connectMaterial(detail, 72); //detail
	sponza->connectMaterial(detail, 73); //detail
	sponza->connectMaterial(detail, 74); //detail

	Material* flagpole = AssetDatabase::LoadMaterial("flagpole");
	sponza->connectMaterial(flagpole, 259); //flagpole
	sponza->connectMaterial(flagpole, 260); //flagpole
	sponza->connectMaterial(flagpole, 261); //flagpole
	sponza->connectMaterial(flagpole, 262); //flagpole
	sponza->connectMaterial(flagpole, 263); //flagpole
	sponza->connectMaterial(flagpole, 264); //flagpole
	sponza->connectMaterial(flagpole, 265); //flagpole
	sponza->connectMaterial(flagpole, 266); //flagpole
	sponza->connectMaterial(flagpole, 267); //flagpole
	sponza->connectMaterial(flagpole, 268); //flagpole
	sponza->connectMaterial(flagpole, 269); //flagpole
	sponza->connectMaterial(flagpole, 270); //flagpole
	sponza->connectMaterial(flagpole, 271); //flagpole
	sponza->connectMaterial(flagpole, 272); //flagpole
	sponza->connectMaterial(flagpole, 273); //flagpole
	sponza->connectMaterial(flagpole, 274); //flagpole
	sponza->connectMaterial(flagpole, 290); //flagpole
	sponza->connectMaterial(flagpole, 291); //flagpole
	sponza->connectMaterial(flagpole, 292); //flagpole
	sponza->connectMaterial(flagpole, 293); //flagpole
	sponza->connectMaterial(flagpole, 294); //flagpole
	sponza->connectMaterial(flagpole, 295); //flagpole
	sponza->connectMaterial(flagpole, 296); //flagpole
	sponza->connectMaterial(flagpole, 297); //flagpole
	sponza->connectMaterial(flagpole, 298); //flagpole
	sponza->connectMaterial(flagpole, 299); //flagpole
	sponza->connectMaterial(flagpole, 300); //flagpole
	sponza->connectMaterial(flagpole, 301); //flagpole
	sponza->connectMaterial(flagpole, 302); //flagpole
	sponza->connectMaterial(flagpole, 303); //flagpole
	sponza->connectMaterial(flagpole, 304); //flagpole
	sponza->connectMaterial(flagpole, 305); //flagpole
	sponza->connectMaterial(flagpole, 306); //flagpole
	sponza->connectMaterial(flagpole, 307); //flagpole
	sponza->connectMaterial(flagpole, 308); //flagpole
	sponza->connectMaterial(flagpole, 309); //flagpole
	sponza->connectMaterial(flagpole, 310); //flagpole
	sponza->connectMaterial(flagpole, 311); //flagpole
	sponza->connectMaterial(flagpole, 312); //flagpole
	sponza->connectMaterial(flagpole, 313); //flagpole
	sponza->connectMaterial(flagpole, 314); //flagpole
	sponza->connectMaterial(flagpole, 315); //flagpole
	sponza->connectMaterial(flagpole, 316); //flagpole
	sponza->connectMaterial(flagpole, 317); //flagpole
	sponza->connectMaterial(flagpole, 318); //flagpole
	sponza->connectMaterial(flagpole, 319); //flagpole
}

void VulkanApp::initOVR() {
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
		return;


	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		return;
	}

	desc = ovr_GetHmdDesc(session);
	resolution = desc.Resolution;

    // Initialize eye rendering information.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    g_EyeFov[0] = desc.DefaultEyeFov[0];
    g_EyeFov[1] = desc.DefaultEyeFov[1];
	camera.setHmdState(bRenderToHmd, g_EyeFov);

	// Configure Stereo settings.
	Sizei recommenedTex0Size = ovr_GetFovTextureSize(session, ovrEye_Left, g_EyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovr_GetFovTextureSize(session, ovrEye_Right, g_EyeFov[1], 1.0f);
	swapSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	swapSize.h = glm::max(recommenedTex0Size.h, recommenedTex1Size.h);
	swapExtent = { uint32_t(swapSize.w), uint32_t(swapSize.h) };

	//get initial head pose
	ovrTrackingState ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrTrue);
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		ovrPosef pose = ts.HeadPose.ThePose;
		Quatf poseQuat = pose.Orientation;
		lastHmdPos = glm::vec3(pose.Position.x, pose.Position.y, pose.Position.z);
		poseQuat.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z, OVR::RotateDirection::Rotate_CCW, OVR::HandedSystem::Handed_R>(&lastHmdEuler.y, &lastHmdEuler.x, &lastHmdEuler.z);
	}
}

void VulkanApp::initOVRLayer() {
	// Initialize VR structures, filling out description.
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef      hmdToEyeViewPose[2];
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
	hmdToEyeViewPose[0] = eyeRenderDesc[0].HmdToEyePose;
	hmdToEyeViewPose[1] = eyeRenderDesc[1].HmdToEyePose;

	// Initialize our single full screen Fov layer.
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = 0;
	layer.ColorTexture[0] = textureSwapChain.textureChain;
	layer.ColorTexture[1] = textureSwapChain.textureChain;
	layer.Fov[0] = eyeRenderDesc[0].Fov;
	layer.Fov[1] = eyeRenderDesc[1].Fov;
	layer.Viewport[0] = Recti(0, 0, swapSize.w / 2, swapSize.h);
	layer.Viewport[1] = Recti(swapSize.w / 2, 0, swapSize.w / 2, swapSize.h);
	// ld.RenderPose and ld.SensorSampleTime are updated later per frame.
}
void VulkanApp::shutdownOVR() {
	ovr_Destroy(session);
	ovr_Shutdown();
}

void VulkanApp::queryHmdOrientationAndPosition() {
	// Query the HMD for ts current tracking state.
	ovrTrackingState ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrTrue);


	//
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		ovrPosef pose = ts.HeadPose.ThePose;
		Quatf poseQuat = pose.Orientation;
		glm::vec3 currHmdPos(pose.Position.x, pose.Position.y, pose.Position.z);
		glm::vec3 currHmdEuler;
		poseQuat.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z, OVR::RotateDirection::Rotate_CCW, OVR::HandedSystem::Handed_R>(&currHmdEuler.y, &currHmdEuler.x, &currHmdEuler.z);
		const glm::vec3 deltaHmdEuler = currHmdEuler - lastHmdEuler;
		const glm::vec3 deltaHmdPos = currHmdPos - lastHmdPos;

		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. hmdToEyePose) may change at runtime.
		ovrEyeRenderDesc eyeRenderDesc[ovrEye_Count];
		for (auto eye : { ovrEye_Left, ovrEye_Right })
			eyeRenderDesc[eye] = ovr_GetRenderDesc(session, eye, desc.DefaultEyeFov[eye]);

		// Get eye poses, feeding in correct IPD offset, the eye poses are relative to the hmd center( the head pose ), previously retrieved.
		ovrPosef HmdToEyePose[ovrEye_Count] = { eyeRenderDesc[ovrEye_Left].HmdToEyePose,
			eyeRenderDesc[ovrEye_Right].HmdToEyePose };
		double sensorSampleTime = 20; // sensorSampleTime is fed into ovr_SubmitFrame later

		//TODO: should use GetEyePoses but need to compile ovr lib with some other visual studio settings in order for the linker to not complain?
		ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, eyeRenderPose, &sensorSampleTime);
		//if GetEyePoses works comment out the ori and pos assignments, leave the renderPose assignment
		//eyeRenderPose[ovrEye_Left].Orientation = pose.Orientation;
		//eyeRenderPose[ovrEye_Right].Orientation = pose.Orientation;
		//eyeRenderPose[ovrEye_Left].Position.x = pose.Position.x + HmdToEyePose[ovrEye_Left].Position.x;
		//eyeRenderPose[ovrEye_Left].Position.y = pose.Position.y + HmdToEyePose[ovrEye_Left].Position.y;
		//eyeRenderPose[ovrEye_Left].Position.z = pose.Position.z + HmdToEyePose[ovrEye_Left].Position.z;
		//eyeRenderPose[ovrEye_Right].Position.x = pose.Position.x + HmdToEyePose[ovrEye_Right].Position.x;
		//eyeRenderPose[ovrEye_Right].Position.y = pose.Position.y + HmdToEyePose[ovrEye_Right].Position.y;
		//eyeRenderPose[ovrEye_Right].Position.z = pose.Position.z + HmdToEyePose[ovrEye_Right].Position.z;
		layer.RenderPose[ovrEye_Left] = eyeRenderPose[ovrEye_Left];
		layer.RenderPose[ovrEye_Right] = eyeRenderPose[ovrEye_Right];
		layer.SensorSampleTime = sensorSampleTime;

		//if the above is figured out change HmdToEyePose to eyeRenderPose(although eyeRenderPose might be absolute pose in the future(ori, pos) not relative to current hmd head

		camera.UpdateOrbitHmdVRSampleCode(eyeRenderPose);


		//glm::vec3 relativeLeftPos(eyeRenderPose[ovrEye_Left].Position.x, eyeRenderPose[ovrEye_Left].Position.y, eyeRenderPose[ovrEye_Left].Position.z);
		//glm::vec3 relativeRightPos(eyeRenderPose[ovrEye_Right].Position.x, eyeRenderPose[ovrEye_Right].Position.y, eyeRenderPose[ovrEye_Right].Position.z);
		////glm::vec3 relativeLeftPos(HmdToEyePose[ovrEye_Left].Position.x, HmdToEyePose[ovrEye_Left].Position.y, HmdToEyePose[ovrEye_Left].Position.z);
		////glm::vec3 relativeRightPos(HmdToEyePose[ovrEye_Right].Position.x, HmdToEyePose[ovrEye_Right].Position.y, HmdToEyePose[ovrEye_Right].Position.z);
		//camera.UpdateOrbitHmdVR(deltaHmdEuler, deltaHmdPos, relativeLeftPos, relativeRightPos);

		lastHmdPos = currHmdPos;
		lastHmdEuler = currHmdEuler;
	}


}
void VulkanApp::initVulkan() {
	camera.setCamera(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), NEAR_PLANE, FAR_PLANE);
	initOVR();

	createInstance();
	setupDebugCallback();
	createSurface();
	//

	if (bRenderToHmd)
		ovr_GetSessionPhysicalDeviceVk(session, luid, instance, &physicalDevice);	
	else
		pickPhysicalDevice();

	createLogicalDevice();

	// Let the compositor know which queue to synchronize with
	ovr_SetSynchonizationQueueVk(session, presentQueue);

	//01. Create Swapchains
	if (bRenderToHmd) {
		swapChainImageFormat = oculusSwapFormat;
		swapChainExtent = swapExtent;
		createFrameBufferRenderPass();
		oculusRenderPass.pass = frameBufferRenderPass;
		textureSwapChain.Create(session, swapExtent, oculusRenderPass, VK_NULL_HANDLE, device);
		//for (auto& tex : textureSwapChain.texElements) {
		swapChainImages.resize(textureSwapChain.texElements.size());
		swapChainImageViews.resize(textureSwapChain.texElements.size());
		swapChainFramebuffers.resize(textureSwapChain.texElements.size());
		for (int i = 0; i < textureSwapChain.texElements.size(); ++i) {
			auto& tex = textureSwapChain.texElements[i];
			swapChainImages[i]			= tex.image;
			swapChainImageViews[i]		= tex.view;
			swapChainFramebuffers[i]	= tex.fb.fb;
		}
		initOVRLayer();
	} else {
		createSwapChain();
		createSwapChainImageViews();
		createFrameBufferRenderPass();
		createFramebuffers();
	}


	//02. Create GlobalImagebuffers
	createGbuffers();
	createSceneBuffer();

	
	//03. Create CommandPool
	//[Stage] PreDraw 
	createDeferredCommandPool();

	
	standardShadow.Initialize(device, physicalDevice, surface, LayerCount, TagQueue, &objectManager);
	standardShadow.setExtent(16384, 4096);
	standardShadow.createImages(VK_FORMAT_R16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	standardShadow.createCommandPool();
	

	//[Stage] Post-process
	PostProcess* VXGIPostProcess = new PostProcess;
	VXGIPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f), false, DRAW_INDEX, 0, false, NULL);
	VXGIPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VXGIPostProcess->createCommandPool();

	PostProcess* sceneImageStage = new PostProcess;
	sceneImageStage->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f), false, DRAW_INDEX, 0, false, NULL);
	sceneImageStage->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	sceneImageStage->createCommandPool();

	sceneStage = sceneImageStage;
	
	PostProcess* HDRHighlightPostProcess = new PostProcess;

	HDRHighlightPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM), false, DRAW_INDEX, 0, false, NULL);
	HDRHighlightPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HDRHighlightPostProcess->createCommandPool();	
	

	PostProcess* HorizontalBlurPostProcess = new PostProcess;
	HorizontalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 2.0f, DOWNSAMPLING_BLOOM * 2.0f), false, DRAW_INDEX, 0, false, NULL);
	HorizontalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HorizontalBlurPostProcess->createCommandPool();


	PostProcess* VerticalBlurPostProcess = new PostProcess;
	VerticalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 4.0f, DOWNSAMPLING_BLOOM * 4.0f), false, DRAW_INDEX, 0, false, NULL);
	VerticalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VerticalBlurPostProcess->createCommandPool();


	PostProcess* HorizontalBlurPostProcess2 = new PostProcess;
	HorizontalBlurPostProcess2->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 8.0f, DOWNSAMPLING_BLOOM * 8.0f), false, DRAW_INDEX, 0, false, NULL);
	HorizontalBlurPostProcess2->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HorizontalBlurPostProcess2->createCommandPool();


	PostProcess* VerticalBlurPostProcess2 = new PostProcess;
	VerticalBlurPostProcess2->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM  * 16.0f, DOWNSAMPLING_BLOOM  * 16.0f), false, DRAW_INDEX, 0, false, NULL);
	VerticalBlurPostProcess2->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VerticalBlurPostProcess2->createCommandPool();

	/*
	PostProcess* HorizontalBlurPostProcess = new PostProcess;
	HorizontalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 2.0f, DOWNSAMPLING_BLOOM * 2.0f), true, DRAW_DISPATCH, 0, false, NULL);
	HorizontalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HorizontalBlurPostProcess->createCommandPool();
	
	
	PostProcess* VerticalBlurPostProcess = new PostProcess;
	VerticalBlurPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 4.0f, DOWNSAMPLING_BLOOM * 4.0f), true, DRAW_DISPATCH, 0, false, NULL);
	VerticalBlurPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VerticalBlurPostProcess->createCommandPool();


	PostProcess* HorizontalBlurPostProcess2 = new PostProcess;
	HorizontalBlurPostProcess2->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM * 8.0f, DOWNSAMPLING_BLOOM * 8.0f), true, DRAW_DISPATCH, 0, false, NULL);
	HorizontalBlurPostProcess2->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	HorizontalBlurPostProcess2->createCommandPool();


	PostProcess* VerticalBlurPostProcess2 = new PostProcess;
	VerticalBlurPostProcess2->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(DOWNSAMPLING_BLOOM  * 8.0f, DOWNSAMPLING_BLOOM  * 8.0f), true, DRAW_DISPATCH, 0, false, NULL);
	VerticalBlurPostProcess2->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VerticalBlurPostProcess2->createCommandPool();
	*/

	

	/*
	PostProcess* VoxelRenderProcess = new PostProcess;
	VoxelRenderProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f), false, DRAW_VERTEX, uint32_t(VOXEL_SIZE*VOXEL_SIZE*VOXEL_SIZE), true, NULL);
	VoxelRenderProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VoxelRenderProcess->createCommandPool();
	*/

	

	
	PostProcess* LastPostProcess = new PostProcess;
	LastPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f), false, DRAW_INDEX, 0, false, NULL);
	LastPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	LastPostProcess->createCommandPool();


	//VR BARREL AND ABERRATION
	/*
	PostProcess* BarrelAndAberrationPostProcess = new PostProcess;
	BarrelAndAberrationPostProcess->Initialize(device, physicalDevice, surface, &swapChainExtent, LayerCount, 1, glm::vec2(1.0f, 1.0f), false, DRAW_INDEX, 0, false, NULL);
	BarrelAndAberrationPostProcess->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	BarrelAndAberrationPostProcess->createCommandPool();
	*/
	//!!! theLastPostProcess has to indicate the last post process, always !!!
	//theLastPostProcess = VoxelRenderProcess;
	theLastPostProcess = LastPostProcess;
	//theLastPostProcess = LastPostProcess;

	postProcessStages.push_back(VXGIPostProcess);

	postProcessStages.push_back(sceneStage);
	postProcessStages.push_back(HDRHighlightPostProcess);

	postProcessStages.push_back(HorizontalBlurPostProcess);
	postProcessStages.push_back(VerticalBlurPostProcess);

	postProcessStages.push_back(HorizontalBlurPostProcess2);
	postProcessStages.push_back(VerticalBlurPostProcess2);
	
	//postProcessStages.push_back(VoxelRenderProcess);

	

	postProcessStages.push_back(LastPostProcess);

	//VR BARREL AND ABERRATION
	//postProcessStages.push_back(BarrelAndAberrationPostProcess);

	//[Stage] Frame buffer
	createFrameBufferCommandPool();
	

	
	//03. Load Assets

	//[Lights]
	DirectionalLight DL01;
	//DL01.lightInfo.lightColor = glm::vec4(0.929412, 0.862745, 0.650980, 1.0);
	DL01.lightInfo.lightColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
	DL01.lightInfo.focusPosition = glm::vec4(-0.5, 14.5, -0.4, 1.0);
	//DL01.lightInfo.lightPosition = glm::vec4(-0.5, 14.5, -0.5, 1.0);

	DL01.lightInfo.lightPosition = glm::vec4(-0.5, 14.618034, -0.4, 1.0);
	//DL01.lightDirection = glm::vec4(glm::normalize(glm::vec3(DL01.lightInfo.focusPosition) - glm::vec3(DL01.lightInfo.lightPosition)), 0.0);
	//SetDirectionLightMatrices(DL01, 18.0f, 10.0f, 0.0f, 20.0f);

	directionLights.push_back(DL01);
	swingMainLight();


	AssetDatabase::GetInstance();

	//[Geometries]
	AssetDatabase::SetDevice(device, physicalDevice, deferredCommandPool, objectDrawQueue);
	

	//[Textures] 
	LoadTextures();


	//[Materials]
	//Object Materials
	LoadObjectMaterials();

	
	//Global Materials
	voxelConetracingMaterial = new VoxelConetracingMaterial;
	voxelConetracingMaterial->setDirectionalLights(&directionLights);
	voxelConetracingMaterial->LoadFromFilename(device, physicalDevice, voxelConetracingMaterial->commandPool, postProcessQueue, "VXGI_material");
	voxelConetracingMaterial->creatDirectionalLightBuffer();
	voxelConetracingMaterial->setShaderPaths("shaders/voxelConeTracing.vert.spv", "shaders/voxelConeTracing.frag.spv", "", "", "", "");
	voxelConetracingMaterial->setScreenScale(VXGIPostProcess->getScreenScale());
	VXGIPostProcess->material = voxelConetracingMaterial;


	lightingMaterial = new LightingMaterial;	
	lightingMaterial->setDirectionalLights(&directionLights);	
	lightingMaterial->LoadFromFilename(device, physicalDevice, sceneStage->commandPool, lightingQueue, "lighting_material");
	lightingMaterial->creatDirectionalLightBuffer();
	lightingMaterial->setShaderPaths("shaders/lighting.vert.spv", "shaders/lighting.frag.spv", "", "", "", "");
	//Link
	sceneStage->material = lightingMaterial;
	
	debugDisplayMaterials.resize(NUM_DEBUGDISPLAY);
	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		debugDisplayMaterials[i] = new DebugDisplayMaterial;
		debugDisplayMaterials[i]->LoadFromFilename(device, physicalDevice, deferredCommandPool, lightingQueue, "debugDisplay_material");
		debugDisplayMaterials[i]->setShaderPaths("shaders/debug.vert.spv", "shaders/debug" + convertToString(static_cast<int>(i)) + ".frag.spv", "", "", "", "");
	}

	//Post Process Materials
	hdrHighlightMaterial = new HDRHighlightMaterial;
	hdrHighlightMaterial->LoadFromFilename(device, physicalDevice, HDRHighlightPostProcess->commandPool, postProcessQueue, "hdrHighlight_material");
	hdrHighlightMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/HDRHighlight.frag.spv", "", "", "", "");
	hdrHighlightMaterial->setScreenScale(glm::vec2(DOWNSAMPLING_BLOOM, DOWNSAMPLING_BLOOM));

	//Link
	HDRHighlightPostProcess->material = hdrHighlightMaterial;


	HBMaterial = new BlurMaterial;
	HBMaterial->LoadFromFilename(device, physicalDevice, HorizontalBlurPostProcess->commandPool, postProcessQueue, "HB_material");
	HBMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/horizontalBlur.frag.spv", "", "", "", "");
	HBMaterial->setScreenScale(HorizontalBlurPostProcess->getScreenScale());
	HorizontalBlurPostProcess->material = HBMaterial;

	VBMaterial = new BlurMaterial;
	VBMaterial->LoadFromFilename(device, physicalDevice, VerticalBlurPostProcess->commandPool, postProcessQueue, "VB_material");
	VBMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/verticalBlur.frag.spv", "", "", "", "");
	VBMaterial->setScreenScale(VerticalBlurPostProcess->getScreenScale());
	VerticalBlurPostProcess->material = VBMaterial;

	HBMaterial2 = new BlurMaterial;
	HBMaterial2->LoadFromFilename(device, physicalDevice, HorizontalBlurPostProcess2->commandPool, postProcessQueue, "HB2_material");
	HBMaterial2->setShaderPaths("shaders/postprocess.vert.spv", "shaders/horizontalBlur.frag.spv", "", "", "", "");
	HBMaterial2->setScreenScale(HorizontalBlurPostProcess2->getScreenScale());
	HorizontalBlurPostProcess2->material = HBMaterial2;

	VBMaterial2 = new BlurMaterial;
	VBMaterial2->LoadFromFilename(device, physicalDevice, VerticalBlurPostProcess2->commandPool, postProcessQueue, "VB2_material");
	VBMaterial2->setShaderPaths("shaders/postprocess.vert.spv", "shaders/verticalBlur.frag.spv", "", "", "", "");
	VBMaterial2->setScreenScale(VerticalBlurPostProcess2->getScreenScale());
	VerticalBlurPostProcess2->material = VBMaterial2;

	/*

	compHBMaterial = new ComputeBlurMaterial;
	compHBMaterial->LoadFromFilename(device, physicalDevice, HorizontalBlurPostProcess->commandPool, postProcessQueue, "compHB_material");
	compHBMaterial->setShaderPaths("", "", "", "", "", "shaders/computeHorizonBlur.comp.spv");
	compHBMaterial->setScreenScale(HorizontalBlurPostProcess->getScreenScale());
	HorizontalBlurPostProcess->material = compHBMaterial;

	compVBMaterial = new ComputeBlurMaterial;
	compVBMaterial->LoadFromFilename(device, physicalDevice, VerticalBlurPostProcess->commandPool, postProcessQueue, "compVB_material");
	compVBMaterial->setShaderPaths("", "", "", "", "", "shaders/computeVerticalBlur.comp.spv");
	compVBMaterial->setScreenScale(VerticalBlurPostProcess->getScreenScale());
	VerticalBlurPostProcess->material = compVBMaterial;


	compHBMaterial2 = new ComputeBlurMaterial;
	compHBMaterial2->LoadFromFilename(device, physicalDevice, HorizontalBlurPostProcess2->commandPool, postProcessQueue, "compHB_material2");
	compHBMaterial2->setShaderPaths("", "", "", "", "", "shaders/computeHorizonBlur.comp.spv");
	compHBMaterial2->setScreenScale(HorizontalBlurPostProcess2->getScreenScale());
	HorizontalBlurPostProcess2->material = compHBMaterial2;

	compVBMaterial2 = new ComputeBlurMaterial;
	compVBMaterial2->LoadFromFilename(device, physicalDevice, VerticalBlurPostProcess2->commandPool, postProcessQueue, "compVzB_material2");
	compVBMaterial2->setShaderPaths("", "", "", "", "", "shaders/computeVerticalBlur.comp.spv");
	compVBMaterial2->setScreenScale(VerticalBlurPostProcess2->getScreenScale());
	VerticalBlurPostProcess2->material = compVBMaterial2;
	*/


	/*
	voxelRenderMaterial = new VoxelRenderMaterial;
	voxelRenderMaterial->LoadFromFilename(device, physicalDevice, VoxelRenderProcess->commandPool, postProcessQueue, "voxelRender_material");
	voxelRenderMaterial->setShaderPaths("shaders/voxelRender.vert.spv", "shaders/voxelRender.frag.spv", "", "", "shaders/voxelRender.geom.spv", "");

	VoxelRenderProcess->material = voxelRenderMaterial;
	*/


	lastPostProcessMaterial = new LastPostProcessgMaterial;
	lastPostProcessMaterial->LoadFromFilename(device, physicalDevice, LastPostProcess->commandPool, postProcessQueue, "lastPostProcess_material");
	lastPostProcessMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/lastPostProcess.frag.spv", "", "", "", "");

	LastPostProcess->material = lastPostProcessMaterial;

	

	//VR BARREL AND ABERRATION
	/*
	BarrelAndAberrationPostProcessMaterial = new HDRHighlightMaterial;
	BarrelAndAberrationPostProcessMaterial->LoadFromFilename(device, physicalDevice, BarrelAndAberrationPostProcess->commandPool, postProcessQueue, "BarrelAndAberrationPostProcess_material");
	BarrelAndAberrationPostProcessMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/BarrelAndAberrationPostProcess.frag.spv", "", "", "", "");
	BarrelAndAberrationPostProcess->material = BarrelAndAberrationPostProcessMaterial;
	*/

	//Frame Buffer Materials
	frameBufferMaterial = new FinalRenderingMaterial;
	frameBufferMaterial->LoadFromFilename(device, physicalDevice, frameBufferCommandPool, presentQueue, "frameDisplay_material");
	frameBufferMaterial->setShaderPaths("shaders/postprocess.vert.spv", "shaders/scene.frag.spv", "", "", "", "");
	
	//[Objects]
	
	
	

	
	Object *Chromie = new Object;

	Chromie->init(device, physicalDevice, deferredCommandPool, objectDrawQueue, "objects/Chromie.obj", 0, false);
	Chromie->scale = glm::vec3(0.1f);
	Chromie->UpdateOrbit(0.0f, 85.0f, 0.0);
	Chromie->position = glm::vec3(3.0, -0.05, -0.25);
	Chromie->update();
	//Chromie->bRoll = true;
	Chromie->rollSpeed = 10.0f;
	Chromie->connectMaterial(AssetDatabase::LoadMaterial("standard_material2"), 0);

	objectManager.push_back(Chromie);

	
	Object *Cerberus = new Object;
	
	Cerberus->init(device, physicalDevice, deferredCommandPool, objectDrawQueue, "objects/Cerberus/Cerberus.obj", 0, false);
	Cerberus->scale = glm::vec3(3.0f);
	Cerberus->position = glm::vec3(0.0, 3.0, -0.25);
	Cerberus->update();
	Cerberus->bRoll = true;
	Cerberus->rollSpeed = 17.0f;
	Cerberus->connectMaterial(AssetDatabase::LoadMaterial("standard_material3"), 0);

	objectManager.push_back(Cerberus);
	
	/*
	Object *Johanna = new Object;
	Johanna->init(device, physicalDevice, deferredCommandPool, objectDrawQueue, "objects/Johanna.obj", 0, false);	
	Johanna->scale = glm::vec3(0.3f);
	Johanna->position = glm::vec3(-1.0, -0.05, -0.25);
	Johanna->update();	
	Johanna->bRoll = true;
	Johanna->rollSpeed = 6.0f;
	Johanna->connectMaterial(AssetDatabase::LoadMaterial("standard_material"), 0);
	objectManager.push_back(Johanna);
	*/

	Object *sponza = new Object;
	sponza->init(device, physicalDevice, deferredCommandPool, objectDrawQueue, "objects/sponza.obj", -1, true);
	sponza->scale = glm::vec3(0.01f);
	sponza->position = glm::vec3(0.0, 0.0, 0.0);
	sponza->update();
	ConnectSponzaMaterials(sponza);
	objectManager.push_back(sponza);
	

	voxelizator.Initialize(device, physicalDevice, surface, LayerCount, uint32_t( floor(log2(VOXEL_SIZE))) , glm::vec2(1.0, 1.0));
	voxelizator.createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	voxelizator.createCommandPool();

	voxelizator.standardObject = sponza;
	//voxelizator.standardObject = Johanna;
	voxelizator.setMatrices();
	
	//voxelizator.createBuffers(20000000);

	glm::vec3 EX = voxelizator.standardObject->AABB.Extents * 2.0f;
	voxelizator.createVoxelInfoBuffer(voxelizator.standardObject->AABB.Center, glm::max(glm::max(EX.x, EX.y), EX.z), VOXEL_SIZE, 0.01f);

	voxelizator.setQueue(objectDrawQueue, TagQueue, AllocationQueue, MipmapQueue);
	voxelizator.initMaterial();
	


	
	//voxelRenderMaterial->createVoxelInfoBuffer(voxelizator.thisObject->AABB.Center, glm::max(glm::max(EX.x, EX.y), EX.z), VOXEL_SIZE);



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

	standardShadow.createRenderPass();

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		objectManager[i]->createShadowMaterial(standardShadow.commandPool, standardShadow.queue, standardShadow.renderPass,
			glm::vec2(static_cast<float>(standardShadow.Extent2D.width), static_cast<float>(standardShadow.Extent2D.height)), glm::vec2(0.0, 0.0), lightingMaterial->shadowConstantBuffer); // !!!!!!!
	}
	
	
	voxelizator.createRenderPass();
	
	

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		if(!postProcessStages[i]->isCSPostProcess)
			postProcessStages[i]->createRenderPass();
	}

	//createFrameBufferRenderPass();


	//06. Create Depth
	createDepthResources();

	standardShadow.createDepthResources();

	//VoxelRenderProcess->createDepthResources();

		
	//07. Create FrameBuffers
	createDeferredFramebuffer();
	standardShadow.createFramebuffer();

	
	voxelizator.createFramebuffer();
	
	

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		if (!postProcessStages[i]->isCSPostProcess)
			postProcessStages[i]->createFramebuffer();
	}


	//createFramebuffers();



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

	
	for (size_t i = 0; i < voxelizator.standardObject->geos.size(); i++)
	{
		VoxelizeMaterial* VXGIMat = voxelizator.VXGIMaterials[i];

		VXGIMat->fragCount = voxelizator.fragCount;
		VXGIMat->setBuffers(voxelizator.voxelFragCountBuffer, voxelizator.ouputPosListBuffer, voxelizator.ouputAlbedoListBuffer);

		VXGIMat->set3DImages(voxelizator.albedo3DImageViewSet[0]);

		VXGIMat->createDescriptorSet();
		VXGIMat->connectRenderPass(voxelizator.renderPass);
		VXGIMat->createGraphicsPipeline(voxelizator.extent2D);
	}

	//[global]
	lightingMaterial->setGbuffers(&gBufferImageViews, depthImageView, standardShadow.outputImageView, VXGIPostProcess->outputImageView);
	lightingMaterial->createDescriptorSet();
	lightingMaterial->connectRenderPass(sceneStage->renderPass);
	lightingMaterial->createGraphicsPipeline(swapChainExtent);

	//[postProcess]
	hdrHighlightMaterial->setImageViews(sceneStage->outputImageView, depthImageView);
	hdrHighlightMaterial->createDescriptorSet();
	hdrHighlightMaterial->connectRenderPass(HDRHighlightPostProcess->renderPass);
	hdrHighlightMaterial->createGraphicsPipeline(glm::vec2(HDRHighlightPostProcess->pExtent2D->width, HDRHighlightPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));

	HBMaterial->setImageViews(HDRHighlightPostProcess->outputImageView, depthImageView);
	HBMaterial->createDescriptorSet();
	HBMaterial->connectRenderPass(HorizontalBlurPostProcess->renderPass);
	HBMaterial->createGraphicsPipeline(glm::vec2(HorizontalBlurPostProcess->pExtent2D->width, HorizontalBlurPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));

	VBMaterial->setImageViews(HorizontalBlurPostProcess->outputImageView, depthImageView);
	VBMaterial->createDescriptorSet();
	VBMaterial->connectRenderPass(VerticalBlurPostProcess->renderPass);
	VBMaterial->createGraphicsPipeline(glm::vec2(VerticalBlurPostProcess->pExtent2D->width, VerticalBlurPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));

	HBMaterial2->setImageViews(VerticalBlurPostProcess->outputImageView, depthImageView);
	HBMaterial2->createDescriptorSet();
	HBMaterial2->connectRenderPass(HorizontalBlurPostProcess2->renderPass);
	HBMaterial2->createGraphicsPipeline(glm::vec2(HorizontalBlurPostProcess2->pExtent2D->width, HorizontalBlurPostProcess2->pExtent2D->height), glm::vec2(0.0, 0.0));

	VBMaterial2->setImageViews(HorizontalBlurPostProcess2->outputImageView, depthImageView);
	VBMaterial2->createDescriptorSet();
	VBMaterial2->connectRenderPass(VerticalBlurPostProcess2->renderPass);
	VBMaterial2->createGraphicsPipeline(glm::vec2(VerticalBlurPostProcess2->pExtent2D->width, VerticalBlurPostProcess2->pExtent2D->height), glm::vec2(0.0, 0.0));

	/*
	compHBMaterial->setImageViews(HDRHighlightPostProcess->outputImageView, HorizontalBlurPostProcess->outputImageView);
	compHBMaterial->createDescriptorSet();
	compHBMaterial->updateDispatchSize(glm::ivec3( 1, static_cast<int>(HorizontalBlurPostProcess->getImageSize().y), 1));
	compHBMaterial->createComputePipeline();

	compVBMaterial->setImageViews(HorizontalBlurPostProcess->outputImageView, VerticalBlurPostProcess->outputImageView);
	compVBMaterial->createDescriptorSet();
	compVBMaterial->updateDispatchSize(glm::ivec3(static_cast<int>(VerticalBlurPostProcess->getImageSize().x), 1, 1));
	compVBMaterial->createComputePipeline();

	compHBMaterial2->setImageViews(VerticalBlurPostProcess->outputImageView, HorizontalBlurPostProcess2->outputImageView);
	compHBMaterial2->createDescriptorSet();
	compHBMaterial2->updateDispatchSize(glm::ivec3(1, static_cast<int>(HorizontalBlurPostProcess2->getImageSize().y), 1));
	compHBMaterial2->createComputePipeline();

	compVBMaterial2->setImageViews(HorizontalBlurPostProcess2->outputImageView, VerticalBlurPostProcess2->outputImageView);
	compVBMaterial2->createDescriptorSet();
	compVBMaterial2->updateDispatchSize(glm::ivec3(static_cast<int>(VerticalBlurPostProcess2->getImageSize().x), 1, 1));
	compVBMaterial2->createComputePipeline();
	*/

	lastPostProcessMaterial->setImageViews(sceneStage->outputImageView, VerticalBlurPostProcess2->outputImageView, depthImageView);
	lastPostProcessMaterial->createDescriptorSet();
	lastPostProcessMaterial->connectRenderPass(LastPostProcess->renderPass);
	lastPostProcessMaterial->createGraphicsPipeline(glm::vec2(LastPostProcess->pExtent2D->width, LastPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));	
	


	//VR BARREL AND ABERRATION
	/*
	BarrelAndAberrationPostProcessMaterial->setImageViews(LastPostProcess->outputImageView, depthImageView);
	BarrelAndAberrationPostProcessMaterial->createDescriptorSet();
	BarrelAndAberrationPostProcessMaterial->connectRenderPass(BarrelAndAberrationPostProcess->renderPass);
	BarrelAndAberrationPostProcessMaterial->createGraphicsPipeline(glm::vec2(BarrelAndAberrationPostProcess->pExtent2D->width, BarrelAndAberrationPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));
	*/

	//[debug]
	for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
	{
		debugDisplayMaterials[i]->setDubugBuffers(&gBufferImageViews, depthImageView, VXGIPostProcess->outputImageView, standardShadow.outputImageView);
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
	
	//frameBufferMaterial->setImageViews(standardShadow.outputImageView, depthImageView);
	
	//frameBufferMaterial->setImageViews(BarrelAndAberrationPostProcess->outputImageView, depthImageView);
	//frameBufferMaterial->setImageViews(voxelizator.outputImageView, depthImageView);
	frameBufferMaterial->createDescriptorSet();
	frameBufferMaterial->connectRenderPass(frameBufferRenderPass);
	frameBufferMaterial->createGraphicsPipeline(glm::vec2(swapChainExtent.width, swapChainExtent.height), glm::vec2(0.0, 0.0));


	voxelizator.createCommandBuffers();
	voxelizator.createSemaphore();

	//Create Semaphore
	createSemaphores();
	standardShadow.createSemaphore();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->createSemaphore();
	}

	//pre-Draw Voxels
	//void* data;
	//voxelizator.createVoxels(camera, objectDrawSemaphore);
	voxelizator.createMipmaps(voxelizator.createVoxels(camera, VK_NULL_HANDLE));

	//Voxel Render
	/*
	VoxelRenderProcess->vertexSize = voxelizator.fragCount;

	voxelRenderMaterial->setfragListVoxelCount(VoxelRenderProcess->vertexSize);
	voxelRenderMaterial->createVertexBuffer();
	voxelRenderMaterial->setBuffer(voxelizator.ouputPosListBuffer, voxelizator.ouputAlbedoListBuffer, voxelizator.OctreeBuffer, voxelizator.voxelInfoBuffer);
	voxelRenderMaterial->maxNode = voxelizator.maxiumOCtreeNodeCount;

	voxelRenderMaterial->setImageviews(voxelizators[i]->albedo3DImageViewSet[0]);
	voxelRenderMaterial->createDescriptorSet();
	voxelRenderMaterial->connectRenderPass(VoxelRenderProcess->renderPass);
	voxelRenderMaterial->createGraphicsPipeline(glm::vec2(VoxelRenderProcess->pExtent2D->width, VoxelRenderProcess->pExtent2D->height), glm::vec2(0.0, 0.0));
	*/

	voxelConetracingMaterial->setImageViews(sceneStage->outputImageView, depthImageView, gBufferImageViews[NORMAL_COLOR], gBufferImageViews[SPECULAR_COLOR], &voxelizator.albedo3DImageViewSet, standardShadow.outputImageView);
	voxelConetracingMaterial->setBuffers(voxelizator.voxelInfoBuffer, lightingMaterial->shadowConstantBuffer);
	voxelConetracingMaterial->createDescriptorSet();
	voxelConetracingMaterial->connectRenderPass(VXGIPostProcess->renderPass);
	voxelConetracingMaterial->createGraphicsPipeline(glm::vec2(VXGIPostProcess->pExtent2D->width, VXGIPostProcess->pExtent2D->height), glm::vec2(0.0, 0.0));
	

	//09. Create CommandBuffers
	createDeferredCommandBuffers();

	standardShadow.createCommandBuffers();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		postProcessStages[i]->offScreenPlane = offScreenPlane;
		postProcessStages[i]->createCommandBuffers();
	}

	createFrameBufferCommandBuffers();	

	updateDrawMode();
}



void VulkanApp::createInstance()
{

	//test call recreateswapchain with vr mode and renderToHmd
	bVRmode = bRenderToHmd;
	camera.vrMode = bVRmode;

	//OCULUS SDK
	//Get the required Vulkan extensions
	ovr_GetInstanceExtensionsVk(luid, extensionNames, &extensionNamesSize);

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

//OCULUS SDK
void ParseExtensionString(char* names, uint32_t& count, const char* const*& arrayPtr) {
	uint32_t extensionCount = 0;
	char* nextExtensionName = names;
	static std::array<const char*, 100> extensionNamePtrs;
	while (*nextExtensionName && (extensionCount < extensionNamePtrs.size()))
	{
		extensionNamePtrs[extensionCount++] = nextExtensionName;
		// Skip to a space or null
		while (*(++nextExtensionName))
		{
			if (*nextExtensionName == ' ')
			{
				// Null-terminate and break out of the loop
				*nextExtensionName++ = '\0';
				break;
			}
		}
	}

	count = extensionCount;
	arrayPtr = &extensionNamePtrs[0];
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

	//OCULUS SDK
	uint32_t count;
	const char* const* arrayPtr;
	ParseExtensionString(extensionNames, count, arrayPtr);
	for (unsigned int i = 0; i < count; i++) {
		extensions.push_back(arrayPtr[i]);
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
	deviceFeatures.geometryShader = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	//OCULUS SDK
	extensionNamesSize = sizeof(extensionNames);
	ovr_GetDeviceExtensionsVk(luid, extensionNames, &extensionNamesSize);
	uint32_t count;
	const char* const* arrayPtr;
	ParseExtensionString(extensionNames, count, arrayPtr);
	for (unsigned int i = 0; i < count; i++) {
		deviceExtensions.push_back(arrayPtr[i]);
	}

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


	vkGetDeviceQueue(device, indices.deferredFamily, 0, &TagQueue);

	vkGetDeviceQueue(device, indices.deferredFamily, 0, &AllocationQueue);

	vkGetDeviceQueue(device, indices.deferredFamily, 0, &MipmapQueue);
	


	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &lightingQueue);

	vkGetDeviceQueue(device, indices.postProcessFamily, 0, &postProcessQueue);

	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

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

	standardShadow.cleanUp();

	//createSwapChain();
	//01. Create Swapchains
	if (bRenderToHmd) {
		swapChainImageFormat = oculusSwapFormat;
		swapChainExtent = swapExtent;
		createFrameBufferRenderPass();
		oculusRenderPass.pass = frameBufferRenderPass; 
		textureSwapChain.Create(session, swapExtent, oculusRenderPass, VK_NULL_HANDLE, device);
		//for (auto& tex : textureSwapChain.texElements) {
		//	swapChainImages.push_back(tex.image);
		//	swapChainImageViews.push_back(tex.view);
		//	swapChainFramebuffers.push_back(tex.fb.fb);
		//}
		swapChainImages.resize(textureSwapChain.texElements.size());
		swapChainImageViews.resize(textureSwapChain.texElements.size());
		swapChainFramebuffers.resize(textureSwapChain.texElements.size());
		for (int i = 0; i < textureSwapChain.texElements.size(); ++i) {
			auto& tex = textureSwapChain.texElements[i];
			swapChainImages[i]			= tex.image;
			swapChainImageViews[i]		= tex.view;
			swapChainFramebuffers[i]	= tex.fb.fb;
		}
		initOVRLayer();
	} else {
		createSwapChain();
		createSwapChainImageViews();
		createFrameBufferRenderPass();
		createFramebuffers();
	}

	createGbuffers();
	createSceneBuffer();
	createImageViews();
	//createSwapChainImageViews();



	
	standardShadow.createImages(VK_FORMAT_R16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//sceneStage->createImages();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];	
		thisPostProcess->vrMode = bVRmode;
		thisPostProcess->createImages();
	}
	

	//REDNER PASS
	createDeferredRenderPass();

	standardShadow.createRenderPass();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];

		if (!postProcessStages[i]->isCSPostProcess)
			thisPostProcess->createRenderPass();
	}
	
	//createFrameBufferRenderPass();
	

	createDepthResources();
	standardShadow.createDepthResources();

	//postProcessStages[7]->createDepthResources();

	//Object Material
	for (size_t i = 0; i < materialManager.size(); i++)
	{
		Material* tempMat = materialManager[i];

		ObjectDrawMaterial* tempObjectDrawMaterial = dynamic_cast<ObjectDrawMaterial *>(tempMat);

		if (tempObjectDrawMaterial != NULL)
		{
			materialManager[i]->vrMode = bVRmode;
			materialManager[i]->connectRenderPass(deferredRenderPass);
			materialManager[i]->createGraphicsPipeline(swapChainExtent);

			continue;
		}
	}

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		objectManager[i]->shadowMaterial->updateDescriptorSet();
		objectManager[i]->shadowMaterial->connectRenderPass(standardShadow.renderPass);
		objectManager[i]->shadowMaterial->createGraphicsPipeline(glm::vec2(standardShadow.Extent2D.width, standardShadow.Extent2D.height), glm::vec2(0.0, 0.0));
	}

	
	voxelConetracingMaterial->vrMode = bVRmode;
	voxelConetracingMaterial->setImageViews(sceneStage->outputImageView, depthImageView, gBufferImageViews[NORMAL_COLOR], gBufferImageViews[SPECULAR_COLOR], &voxelizator.albedo3DImageViewSet, standardShadow.outputImageView);
	voxelConetracingMaterial->updateDescriptorSet();
	voxelConetracingMaterial->connectRenderPass(postProcessStages[0]->renderPass);
	voxelConetracingMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[0]->pExtent2D->width, postProcessStages[0]->pExtent2D->height), glm::vec2(0.0, 0.0));

	
	lightingMaterial->vrMode = bVRmode;
	lightingMaterial->setGbuffers(&gBufferImageViews, depthImageView, standardShadow.outputImageView, postProcessStages[0]->outputImageView );
	lightingMaterial->updateDescriptorSet();
	lightingMaterial->connectRenderPass(sceneStage->renderPass);
	lightingMaterial->createGraphicsPipeline(swapChainExtent);


		for (size_t i = 0; i < NUM_DEBUGDISPLAY; i++)
		{
			debugDisplayMaterials[i]->vrMode = bVRmode;
			debugDisplayMaterials[i]->setDubugBuffers(&gBufferImageViews, depthImageView, postProcessStages[0]->outputImageView, standardShadow.outputImageView);
			debugDisplayMaterials[i]->updateDescriptorSet();
			debugDisplayMaterials[i]->connectRenderPass(frameBufferRenderPass);
		}

		float debugWidth = swapChainExtent.width * 0.25f;
		float debugHeight = swapChainExtent.height * 0.25f;

		if (bVRmode)
			debugWidth *= 0.5f;

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
	hdrHighlightMaterial->vrMode = bVRmode;
	hdrHighlightMaterial->setImageViews(sceneStage->outputImageView, depthImageView);
	hdrHighlightMaterial->updateDescriptorSet();
	hdrHighlightMaterial->connectRenderPass(postProcessStages[2]->renderPass);
	hdrHighlightMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[2]->pExtent2D->width, postProcessStages[2]->pExtent2D->height), glm::vec2(0.0, 0.0));

	HBMaterial->vrMode = bVRmode;
	HBMaterial->setImageViews(postProcessStages[2]->outputImageView, depthImageView);
	HBMaterial->updateDescriptorSet();
	HBMaterial->connectRenderPass(postProcessStages[3]->renderPass);
	HBMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[3]->pExtent2D->width, postProcessStages[3]->pExtent2D->height), glm::vec2(0.0, 0.0));

	VBMaterial->vrMode = bVRmode;
	VBMaterial->setImageViews(postProcessStages[3]->outputImageView, depthImageView);
	VBMaterial->updateDescriptorSet();
	VBMaterial->connectRenderPass(postProcessStages[4]->renderPass);
	VBMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[4]->pExtent2D->width, postProcessStages[4]->pExtent2D->height), glm::vec2(0.0, 0.0));

	HBMaterial2->vrMode = bVRmode;
	HBMaterial2->setImageViews(postProcessStages[4]->outputImageView, depthImageView);
	HBMaterial2->updateDescriptorSet();
	HBMaterial2->connectRenderPass(postProcessStages[5]->renderPass);
	HBMaterial2->createGraphicsPipeline(glm::vec2(postProcessStages[5]->pExtent2D->width, postProcessStages[5]->pExtent2D->height), glm::vec2(0.0, 0.0));

	VBMaterial2->vrMode = bVRmode;
	VBMaterial2->setImageViews(postProcessStages[5]->outputImageView, depthImageView);
	VBMaterial2->updateDescriptorSet();
	VBMaterial2->connectRenderPass(postProcessStages[6]->renderPass);
	VBMaterial2->createGraphicsPipeline(glm::vec2(postProcessStages[6]->pExtent2D->width, postProcessStages[6]->pExtent2D->height), glm::vec2(0.0, 0.0));

	/*
	compHBMaterial->vrMode = bVRmode;
	compHBMaterial->setImageViews(postProcessStages[2]->outputImageView, postProcessStages[3]->outputImageView);
	compHBMaterial->updateDescriptorSet();
	compHBMaterial->updateDispatchSize(glm::ivec3(1, static_cast<int>(postProcessStages[3]->getImageSize().y), 1));
	compHBMaterial->createComputePipeline();

	compVBMaterial->vrMode = bVRmode;
	compVBMaterial->setImageViews(postProcessStages[3]->outputImageView, postProcessStages[4]->outputImageView);
	compVBMaterial->updateDescriptorSet();
	compVBMaterial->updateDispatchSize(glm::ivec3(static_cast<int>(postProcessStages[4]->getImageSize().x), 1, 1));
	compVBMaterial->createComputePipeline();

	compHBMaterial2->vrMode = bVRmode;
	compHBMaterial2->setImageViews(postProcessStages[4]->outputImageView, postProcessStages[5]->outputImageView);
	compHBMaterial2->updateDescriptorSet();
	compHBMaterial2->updateDispatchSize(glm::ivec3(1, static_cast<int>(postProcessStages[5]->getImageSize().y), 1));
	compHBMaterial2->createComputePipeline();

	compVBMaterial2->vrMode = bVRmode;
	compVBMaterial2->setImageViews(postProcessStages[5]->outputImageView, postProcessStages[6]->outputImageView);
	compVBMaterial2->updateDescriptorSet();
	compVBMaterial2->updateDispatchSize(glm::ivec3(static_cast<int>(postProcessStages[6]->getImageSize().x), 1, 1));
	compVBMaterial2->createComputePipeline();
	*/

	/*
	voxelRenderMaterial->vrMode = bVRmode;

	voxelRenderMaterial->updateDescriptorSet();
	voxelRenderMaterial->connectRenderPass(postProcessStages[7]->renderPass);
	voxelRenderMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[7]->pExtent2D->width, postProcessStages[7]->pExtent2D->height), glm::vec2(0.0, 0.0));
	*/

	


	lastPostProcessMaterial->vrMode = bVRmode;
	lastPostProcessMaterial->setImageViews(sceneStage->outputImageView, postProcessStages[6]->outputImageView, depthImageView);
	lastPostProcessMaterial->updateDescriptorSet();
	lastPostProcessMaterial->connectRenderPass(postProcessStages[7]->renderPass);
	lastPostProcessMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[7]->pExtent2D->width, postProcessStages[7]->pExtent2D->height), glm::vec2(0.0, 0.0));

	

	//VR BARREL AND ABERRATION
	/*
	BarrelAndAberrationPostProcessMaterial->vrMode = bVRmode;
	BarrelAndAberrationPostProcessMaterial->setImageViews(postProcessStages[7]->outputImageView, depthImageView);
	BarrelAndAberrationPostProcessMaterial->updateDescriptorSet();
	BarrelAndAberrationPostProcessMaterial->connectRenderPass(postProcessStages[8]->renderPass);
	BarrelAndAberrationPostProcessMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[8]->pExtent2D->width, postProcessStages[8]->pExtent2D->height), glm::vec2(0.0, 0.0));
	
	
	//VR BARREL AND ABERRATION
	BarrelAndAberrationPostProcessMaterial->vrMode = bVRmode;
	BarrelAndAberrationPostProcessMaterial->setImageViews(postProcessStages[6]->outputImageView, depthImageView);
	BarrelAndAberrationPostProcessMaterial->updateDescriptorSet();
	BarrelAndAberrationPostProcessMaterial->connectRenderPass(postProcessStages[7]->renderPass);
	BarrelAndAberrationPostProcessMaterial->createGraphicsPipeline(glm::vec2(postProcessStages[7]->pExtent2D->width, postProcessStages[7]->pExtent2D->height), glm::vec2(0.0, 0.0));
	*/

	frameBufferMaterial->vrMode = bVRmode;
	//frameBufferMaterial->setImageViews(postProcessStages.back()->outputImageView, depthImageView);
	frameBufferMaterial->setImageViews(theLastPostProcess->outputImageView, depthImageView);
	//frameBufferMaterial->setImageViews(standardShadow.outputImageView, depthImageView);
	//frameBufferMaterial->setImageViews(voxelizator.outputImageView, depthImageView);
	
	frameBufferMaterial->updateDescriptorSet();
	frameBufferMaterial->connectRenderPass(frameBufferRenderPass);
	frameBufferMaterial->createGraphicsPipeline(glm::vec2(swapChainExtent.width, swapChainExtent.height), glm::vec2(0.0, 0.0));


	createDeferredFramebuffer();
	createDeferredCommandBuffers();
	
	standardShadow.createFramebuffer();
	standardShadow.createCommandBuffers();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];

		if (!postProcessStages[i]->isCSPostProcess)
			thisPostProcess->createFramebuffer();

		thisPostProcess->createCommandBuffers();

	}
	//createFramebuffers();
	createFrameBufferCommandBuffers();
}

void VulkanApp::createGbuffers()
{
	gBufferImages.resize(NUM_GBUFFERS);
	gBufferImageMemories.resize(NUM_GBUFFERS);

	for (uint32_t i = 0; i < gBufferImages.size(); i++)
	{
		
		if (bVRmode)
		{
			if (i == NORMAL_COLOR)
			{
				createImage(swapChainExtent.width / 2, swapChainExtent.height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImages[i], gBufferImageMemories[i]);
			}
			else
			{
				createImage(swapChainExtent.width / 2, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImages[i], gBufferImageMemories[i]);
			}

			
		}
		else
		{
			if (i == NORMAL_COLOR)
			{
				createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImages[i], gBufferImageMemories[i]);
			}
			else
			{
				createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImages[i], gBufferImageMemories[i]);
			}

			
		}

		

	}
}

void VulkanApp::createSceneBuffer()
{
	if (bVRmode)
	{
		createImage(swapChainExtent.width / 2, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sceneImage, sceneImageMemories);
	}
	else
	{
		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sceneImage, sceneImageMemories);
	}
}

void VulkanApp::createImageViews()
{
	gBufferImageViews.resize(gBufferImages.size());

	//G-buffers
	for (uint32_t i = 0; i < gBufferImages.size(); i++)
	{
		if (i == NORMAL_COLOR)
		{
			gBufferImageViews[i] = createImageView(gBufferImages[i], VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
			gBufferImageViews[i] = createImageView(gBufferImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	sceneImageView = createImageView(sceneImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
}
void VulkanApp::createSwapChainImageViews() {
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

void VulkanApp::transitionMipmapImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange mipSubRange, VkCommandPool commandPool)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange = mipSubRange;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
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

	if (bVRmode)
	{
		createImage(swapChainExtent.width/2, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	else
	{
		createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	

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
	normalColorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

	if (bVRmode)
	{
		fbufCreateInfo.width = swapChainExtent.width/2;
	}
	else
	{
		fbufCreateInfo.width = swapChainExtent.width;
	}

	
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

	if (bVRmode)
	{
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.renderArea.extent.width /= 2;		
	}
	else
	{
		renderPassInfo.renderArea.extent = swapChainExtent;
	}

	
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(deferredCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	for (size_t j = 0; j < objectManager.size(); j++)
	{
		Object *thisObject = objectManager[j];

		for (size_t k = 0; k < thisObject->geos.size(); k++)
		{
			{
				vkCmdBindPipeline(deferredCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thisObject->materials[k]->pipeline);

				VkBuffer vertexBuffers[] = { thisObject->geos[k]->vertexBuffer };
				VkBuffer indexBuffer = thisObject->geos[k]->indexBuffer;
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindDescriptorSets(deferredCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thisObject->materials[k]->pipelineLayout, 0, 1, &thisObject->materials[k]->descriptorSet, 0, nullptr);

				vkCmdBindVertexBuffers(deferredCommandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(deferredCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(deferredCommandBuffer, static_cast<uint32_t>(thisObject->geos[k]->indices.size()), 1, 0, 0, 0);
			}
		}
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
	//Left and Main
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

	//Right
	{
		frameBufferCommandBuffers2.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = frameBufferCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)frameBufferCommandBuffers2.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, frameBufferCommandBuffers2.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (size_t i = 0; i < frameBufferCommandBuffers2.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			vkBeginCommandBuffer(frameBufferCommandBuffers2[i], &beginInfo);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = frameBufferRenderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];

			renderPassInfo.renderArea.extent = swapChainExtent;
			renderPassInfo.renderArea.extent.width /= 2;

			renderPassInfo.renderArea.offset = { static_cast<int32_t>(renderPassInfo.renderArea.extent.width), 0 };
			

			std::array<VkClearValue, 1> clearValues = {};
			clearValues[0].color = { 0.0f, 0.678431f, 0.902f, 1.0f };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(frameBufferCommandBuffers2[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			
			vkCmdBindPipeline(frameBufferCommandBuffers2[i], VK_PIPELINE_BIND_POINT_GRAPHICS, frameBufferMaterial->pipeline2);
			

			VkBuffer vertexBuffers[] = { offScreenPlane->vertexBuffer };
			VkBuffer indexBuffer = offScreenPlane->indexBuffer;
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindDescriptorSets(frameBufferCommandBuffers2[i], VK_PIPELINE_BIND_POINT_GRAPHICS, frameBufferMaterial->pipelineLayout, 0, 1, &frameBufferMaterial->descriptorSet, 0, nullptr);

			vkCmdBindVertexBuffers(frameBufferCommandBuffers2[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(frameBufferCommandBuffers2[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(frameBufferCommandBuffers2[i], static_cast<uint32_t>(offScreenPlane->indices.size()), 1, 0, 0, 0);

			
			if (bDeubDisply)
			{
				for (size_t k = 0; k < NUM_DEBUGDISPLAY; k++)
				{
					vkCmdBindPipeline(frameBufferCommandBuffers2[i], VK_PIPELINE_BIND_POINT_GRAPHICS, debugDisplayMaterials[k]->pipeline2);

					VkBuffer vertexBuffers[] = { debugDisplayPlane->vertexBuffer };
					VkBuffer indexBuffer = debugDisplayPlane->indexBuffer;
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindDescriptorSets(frameBufferCommandBuffers2[i], VK_PIPELINE_BIND_POINT_GRAPHICS, debugDisplayMaterials[k]->pipelineLayout, 0, 1, &debugDisplayMaterials[k]->descriptorSet, 0, nullptr);

					vkCmdBindVertexBuffers(frameBufferCommandBuffers2[i], 0, 1, vertexBuffers, offsets);
					vkCmdBindIndexBuffer(frameBufferCommandBuffers2[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdDrawIndexed(frameBufferCommandBuffers2[i], static_cast<uint32_t>(debugDisplayPlane->indices.size()), 1, 0, 0, 0);
				}
			}
			

			vkCmdEndRenderPass(frameBufferCommandBuffers2[i]);

			if (vkEndCommandBuffer(frameBufferCommandBuffers2[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}
}

void VulkanApp::drawFrame(float deltaTime) {

	uint32_t imageIndex;
	int result;// = -49340534;
	if (bRenderToHmd) {
		int index;
		ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChain.textureChain, &index);//couldnt cast and take address of imageIndex
		if (index > 0 && index < textureSwapChain.texElements.size()) { result = VK_SUCCESS; } 
		//result = ovr_WaitToBeginFrame(session, 0);
		imageIndex = index;
	} else {
		result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), objectDrawSemaphore, VK_NULL_HANDLE, &imageIndex);
	}
	// Get next available index of the texture swap chain

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		reCreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	if (bRenderToHmd) {//make sure we start right before vsync to get a "running start"(valve) or "adaptive queue ahead" (oculus)
		//result = ovr_BeginFrame(session, 0);
	}
	
	updateUniformBuffers(0, deltaTime);

	//objectDrawQueue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore firstWaitSemaphores[] = { objectDrawSemaphore };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	if (bRenderToHmd) {
		//we've already done WaitToBeginFrame so there should be sync issues when useing no semaphore
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
	} else {
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = firstWaitSemaphores;
	}
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

	VkSemaphore prevSemaphore = imageAvailableSemaphore;
	VkSemaphore currentSemaphore;

	//DrawShadow

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &prevSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &standardShadow.commandBuffer;

	currentSemaphore = standardShadow.semaphore;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &currentSemaphore;

	if (vkQueueSubmit(standardShadow.queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	prevSemaphore = currentSemaphore;


	//prevSemaphore = voxelizator.createMipmaps(prevSemaphore);


	//postProcessQueue
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

	VkSemaphore postProcessSignalSemaphores[] = { postProcessSemaphore };

	//frameQueue
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &currentSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frameBufferCommandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = postProcessSignalSemaphores;

	vkQueueWaitIdle(presentQueue);
	if (vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	if (bRenderToHmd) {
		//sync to lastpostprocess->material->queue? or present queue?
		//so basically same as the else case except a command buffer that uses the textureSwapchain stuff(frame buffer)
		// Commit the changes to the texture swap chain
		ovr_CommitTextureSwapChain(session, textureSwapChain.textureChain);
	}

	if (bVRmode)
	{
		updateUniformBuffers(1, deltaTime);

		//objectDrawQueue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		//VkSemaphore firstWaitSemaphores[] = { objectDrawSemaphore };

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = postProcessSignalSemaphores;
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

		for (size_t i = 0; i < postProcessStages.size(); i++)
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
		submitInfo.pCommandBuffers = &frameBufferCommandBuffers2[imageIndex];

		VkSemaphore postProcessSignalSemaphores[] = { postProcessSemaphore };
		if (bRenderToHmd) {
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
		} else {
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = postProcessSignalSemaphores;
		}
		if (vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		if (bRenderToHmd) {
			//sync to lastpostprocess->material->queue? or present queue?
			//so basically same as the else case except a command buffer that uses the textureSwapchain stuff(frame buffer)
			// Commit the changes to the texture swap chain
			ovr_CommitTextureSwapChain(session, textureSwapChain.textureChain);
		}

	}

	if (bRenderToHmd) {
        // Submit rendered eyes as an EyeFov layer
        //ovrLayerEyeFov ld;
        //ld.Header.Type  = ovrLayerType_EyeFov;
        //ld.Header.Flags = 0;
        //ld.SensorSampleTime  = sensorSampleTime;
        //for (auto eye: { ovrEye_Left, ovrEye_Right })
        //{
        //    ld.ColorTexture[eye] = perEye[eye].tex.textureChain;
        //    ld.Viewport[eye]     = perEye[eye].tex.GetViewport();
        //    ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];
        //    ld.RenderPose[eye]   = eyeRenderPose[eye];
        //}
        //ovrLayerHeader* layers = &ld.Header;
        //ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
		//const ovrTrackingState ts = 
		//ovr_CalcEyePoses(ts.HeadPose.ThePose, mHmdToEyeOffsets, mMainLayer.RenderPose);
        ovrLayerHeader* layers = &layer.Header;
        ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

		//result = ovr_EndFrame(session, 0, nullptr, &layers, 1);
	} else {
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
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		vkQueueWaitIdle(presentQueue);
	}
	++frameIndex;
}

void VulkanApp::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &objectDrawSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &postProcessSemaphore) != VK_SUCCESS || 
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &mipMapStartSemaphore) != VK_SUCCESS 
		) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

void VulkanApp::run()
{
	initWindow();	
	initVulkan();

	_oldTime = startTime = std::chrono::high_resolution_clock::now();

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

			fps = static_cast<int>(fpstracker / (currentTime - oldTime));
			fpstracker = 0;
			oldTime = currentTime;

			std::string title = "VR Project | " + convertToString(fps) + " fps | " + convertToString(1000.0 / (double)fps) + " ms";
			glfwSetWindowTitle(window, title.c_str());
		}
		
		auto currentTime = std::chrono::high_resolution_clock::now();
		totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() * 0.001f;

		deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _oldTime).count() * 0.001f;

		
		getAsynckeyState();
		drawFrame(static_cast<float>(deltaTime));

		_oldTime = currentTime;
	}

	vkDeviceWaitIdle(device);
}

void VulkanApp::updateUniformBuffers(unsigned int EYE, float deltaTime)
{
	if (bRotateMainLight)
	{
		mainLightAngle += static_cast<float>(deltaTime) * 0.2f;
		swingMainLight();
	}
	
	
	SetDirectionLightMatrices(directionLights[0], 19.0f, 5.0f, 0.0f, 20.0f);

	autoCameraMoving();

	UniformBufferObject ubo = {};
	
	ubo.modelMat = glm::mat4(1.0);
	
	

	if (bVRmode)
	{
		ubo.viewMat = camera.viewMatforVR[EYE];
		ubo.projMat = camera.projMatforVR[EYE];
		ubo.viewProjMat = camera.viewProjMatforVR[EYE];
		ubo.InvViewProjMat = camera.InvViewProjMatforVR[EYE];
		ubo.modelViewProjMat = ubo.viewProjMat;
		ubo.cameraWorldPos = camera.positionforVR[EYE];
	}
	else
	{
		ubo.viewMat = camera.viewMat;
		ubo.projMat = camera.projMat;
		ubo.viewProjMat = camera.viewProjMat;
		ubo.InvViewProjMat = camera.InvViewProjMat;
		ubo.modelViewProjMat = ubo.viewProjMat;
		ubo.cameraWorldPos = camera.position;
	}
	
	ubo.InvTransposeMat = ubo.modelMat;
	

	ShadowUniformBuffer subo = {};
	subo.viewProjMat = directionLights[0].projMat * directionLights[0].viewMat;
	subo.invViewProjMat = glm::inverse(subo.viewProjMat);	

	void* shadowdata;
	vkMapMemory(device, lightingMaterial->shadowConstantBufferMemory, 0, sizeof(ShadowUniformBuffer), 0, &shadowdata);
	memcpy(shadowdata, &subo, sizeof(ShadowUniformBuffer));
	vkUnmapMemory(device, lightingMaterial->shadowConstantBufferMemory);

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		Object *thisObject = objectManager[i];

		if(thisObject->bRoll)
			thisObject->UpdateOrbit(deltaTime * thisObject->rollSpeed, 0.0f, 0.0f);
		
		ubo.modelMat = thisObject->modelMat;
		ubo.modelViewProjMat = ubo.viewProjMat * thisObject->modelMat;

		glm::mat4 A = ubo.modelMat;
		A[3] = glm::vec4(0, 0, 0, 1);
		ubo.InvTransposeMat = glm::transpose(glm::inverse(A));


		//shadow
		{
			void* data;
			//vkMapMemory(device, thisObject->shadowMaterial->ShadowConstantBufferMemory, 0, sizeof(ShadowUniformBuffer), 0, &data);
			//memcpy(data, &subo, sizeof(ShadowUniformBuffer));
			//vkUnmapMemory(device, thisObject->shadowMaterial->ShadowConstantBufferMemory);


			ObjectUniformBuffer obu = {};
			obu.modelMat = thisObject->modelMat;

			vkMapMemory(device, thisObject->shadowMaterial->objectUniformMemory, 0, sizeof(ObjectUniformBuffer), 0, &data);
			memcpy(data, &obu, sizeof(ObjectUniformBuffer));
			vkUnmapMemory(device, thisObject->shadowMaterial->objectUniformMemory);


		}

		for (size_t k = 0; k < thisObject->materials.size(); k++)
		{
			void* data;
			vkMapMemory(device, thisObject->materials[k]->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
			memcpy(data, &ubo, sizeof(UniformBufferObject));
			vkUnmapMemory(device, thisObject->materials[k]->uniformBufferMemory);

		}
		
	}

	

		{
		UniformBufferObject offScreenUbo = {};

		offScreenUbo.modelMat = glm::mat4(1.0);

		offScreenUbo.viewMat = ubo.viewMat;
		offScreenUbo.projMat = ubo.projMat;
		offScreenUbo.viewProjMat = ubo.viewProjMat;
		offScreenUbo.InvViewProjMat = ubo.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = ubo.cameraWorldPos;

		offScreenPlane->updateVertexBuffer(offScreenUbo.InvViewProjMat);
		offScreenPlaneforPostProcess->updateVertexBuffer(offScreenUbo.InvViewProjMat);

		void* data;
		vkMapMemory(device, lightingMaterial->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &offScreenUbo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, lightingMaterial->uniformBufferMemory);
		
		void* directionLightsData;
		
		vkMapMemory(device, voxelConetracingMaterial->directionalLightBufferMemory, 0, sizeof(DirectionalLight) * directionLights.size(), 0, &directionLightsData);
		memcpy(directionLightsData, &directionLights[0], sizeof(DirectionalLight) * directionLights.size());
		vkUnmapMemory(device, voxelConetracingMaterial->directionalLightBufferMemory);
		
		vkMapMemory(device, lightingMaterial->directionalLightBufferMemory, 0, sizeof(DirectionalLight) * directionLights.size(), 0, &directionLightsData);
		memcpy(directionLightsData, &directionLights[0], sizeof(DirectionalLight) * directionLights.size());
		vkUnmapMemory(device, lightingMaterial->directionalLightBufferMemory);
	}
	
	if (bDeubDisply)
	{
		{
			UniformBufferObject debugDisplayUbo = {};

			debugDisplayUbo.modelMat = glm::mat4(1.0);
			debugDisplayUbo.viewMat = ubo.viewMat;
			debugDisplayUbo.projMat = ubo.projMat;
			debugDisplayUbo.viewProjMat = ubo.viewProjMat;
			debugDisplayUbo.InvViewProjMat = ubo.InvViewProjMat;
			debugDisplayUbo.modelViewProjMat = debugDisplayUbo.viewProjMat;
			debugDisplayUbo.InvTransposeMat = debugDisplayUbo.modelMat;
			debugDisplayUbo.cameraWorldPos = ubo.cameraWorldPos;

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

		VoxelRenderMaterial* isVXGIMat = dynamic_cast<VoxelRenderMaterial*>(postProcessStages[i]->material);

		if (isVXGIMat != NULL)
		{
			offScreenUbo.modelMat = voxelizator.standardObject->modelMat;
		}

		offScreenUbo.viewMat = ubo.viewMat;
		offScreenUbo.projMat = ubo.projMat;
		offScreenUbo.viewProjMat = ubo.viewProjMat;
		offScreenUbo.InvViewProjMat = ubo.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat * offScreenUbo.modelMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = ubo.cameraWorldPos;

		offScreenPlaneforPostProcess->updateVertexBuffer(offScreenUbo.InvViewProjMat);

		void* data;
		vkMapMemory(device, postProcessStages[i]->material->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
		memcpy(data, &offScreenUbo, sizeof(UniformBufferObject));
		vkUnmapMemory(device, postProcessStages[i]->material->uniformBufferMemory);

		BlurMaterial* isBlurMat = dynamic_cast<BlurMaterial*>(postProcessStages[i]->material);
		ComputeBlurMaterial* isComputeBlurMat = dynamic_cast<ComputeBlurMaterial*>(postProcessStages[i]->material);

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
		else if(isComputeBlurMat != NULL)
		{
			BlurUniformBufferObject blurUbo;

			if (bVRmode)
			{
				blurUbo.widthGap = postProcessStages[i]->getImageSize().x * 0.5f;
			}
			else
			{
				blurUbo.widthGap = postProcessStages[i]->getImageSize().x;
				
			}

			blurUbo.heightGap = postProcessStages[i]->getImageSize().y;

			void* data;
			vkMapMemory(device, isComputeBlurMat->blurUniformBufferMemory, 0, sizeof(BlurUniformBufferObject), 0, &data);
			memcpy(data, &blurUbo, sizeof(BlurUniformBufferObject));
			vkUnmapMemory(device, isComputeBlurMat->blurUniformBufferMemory);
		}
		
	}

	{
		UniformBufferObject offScreenUbo = {};

		offScreenUbo.modelMat = glm::mat4(1.0);
		offScreenUbo.viewMat = ubo.viewMat;
		offScreenUbo.projMat = ubo.projMat;
		offScreenUbo.viewProjMat = ubo.viewProjMat;
		offScreenUbo.InvViewProjMat = ubo.InvViewProjMat;
		offScreenUbo.modelViewProjMat = offScreenUbo.viewProjMat;
		offScreenUbo.InvTransposeMat = offScreenUbo.modelMat;
		offScreenUbo.cameraWorldPos = ubo.cameraWorldPos;

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
	vkFreeCommandBuffers(device, frameBufferCommandPool, static_cast<uint32_t>(frameBufferCommandBuffers2.size()), frameBufferCommandBuffers2.data());
	
	for (size_t i = 0; i < materialManager.size(); i++)
	{
		materialManager[i]->cleanPipeline();
	}

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		objectManager[i]->shadowMaterial->cleanPipeline();
	}

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
		thisPostProcess->material->cleanPipeline();
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

	standardShadow.cleanUp();

	AssetDatabase::GetInstance()->cleanUp();

	for (size_t i = 0; i < objectManager.size(); i++)
	{
		delete objectManager[i];
	}

	delete lightingMaterial;
	delete hdrHighlightMaterial;


	delete HBMaterial;
	delete VBMaterial;
	delete HBMaterial2;
	delete VBMaterial2;

	/*
	delete compHBMaterial;
	delete compVBMaterial;
	delete compHBMaterial2;
	delete compVBMaterial2;
	*/

	delete lastPostProcessMaterial;

	//if (standShadowMaterial != NULL)
	//	delete standShadowMaterial;

	if(voxelRenderMaterial != NULL)
		delete voxelRenderMaterial;

	delete voxelConetracingMaterial;
	//delete BarrelAndAberrationPostProcessMaterial;

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
	vkDestroySemaphore(device, mipMapStartSemaphore, nullptr);

	vkDestroyCommandPool(device, frameBufferCommandPool, nullptr);

	standardShadow.shutDown();

	for (size_t i = 0; i < postProcessStages.size(); i++)
	{
		PostProcess* thisPostProcess = postProcessStages[i];
		vkDestroyCommandPool(device, thisPostProcess->commandPool, nullptr);
		delete thisPostProcess;
	}

	vkDestroyRenderPass(device, voxelizator.renderPass, nullptr);
	vkDestroyCommandPool(device, voxelizator.commandPool, nullptr);
	voxelizator.shutDown();
	
	vkDestroyCommandPool(device, deferredCommandPool, nullptr);

	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();

	shutdownOVR();
	
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