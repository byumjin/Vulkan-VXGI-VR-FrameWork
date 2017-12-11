#pragma once

#include "../core/Common.h"
#include "../assets/Geometry.h"

#include "../assets/Material.h"
#include "../assets/Texture.h"

#include "Actor.h"

class Object : public Actor
{
public:
	Object();
	~Object()
	{
		delete shadowMaterial;

		for (size_t i = 0; i < geos.size(); i++)
		{
			delete geos[i];
		}
	}

	void init(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam, int materialOffsetParam, bool needUflipCorrection);

	void loadObjectFromFile(std::string path);

	void initiation(std::string objectNameParam, Geo* geoParam);

	void connectMaterial(Material* mat, unsigned int matIndex);

	void setAABB()
	{
		AABB = geos[0]->AABB;

		for (size_t k = 1; k < geos.size(); k++)
		{
			AABB.maxPt = glm::max(AABB.maxPt, geos[k]->AABB.maxPt);
			AABB.minPt = glm::min(AABB.minPt, geos[k]->AABB.minPt);
		}

		AABB.Center = (AABB.maxPt + AABB.minPt) * 0.5f;
		AABB.Extents = glm::abs(AABB.maxPt - AABB.minPt) * 0.5f;

		AABB.Corners[0] = AABB.Center + glm::vec3(-AABB.Extents.x, -AABB.Extents.y, -AABB.Extents.z);
		AABB.Corners[1] = AABB.Center + glm::vec3(AABB.Extents.x, -AABB.Extents.y, -AABB.Extents.z);
		AABB.Corners[2] = AABB.Center + glm::vec3(-AABB.Extents.x, AABB.Extents.y, -AABB.Extents.z);
		AABB.Corners[3] = AABB.Center + glm::vec3(-AABB.Extents.x, -AABB.Extents.y, AABB.Extents.z);

		AABB.Corners[4] = AABB.Center + glm::vec3(AABB.Extents.x, AABB.Extents.y, -AABB.Extents.z);
		AABB.Corners[5] = AABB.Center + glm::vec3(AABB.Extents.x, -AABB.Extents.y, AABB.Extents.z);
		AABB.Corners[6] = AABB.Center + glm::vec3(-AABB.Extents.x, AABB.Extents.y, AABB.Extents.z);
		AABB.Corners[7] = AABB.Center + glm::vec3(AABB.Extents.x, AABB.Extents.y, AABB.Extents.z);

	}

	void createShadowMaterial(VkCommandPool shadowCommandPool, VkQueue ShadowQueue, VkRenderPass standardShadowRenderPass, glm::vec2 extent, glm::vec2 offset, VkBuffer shadowConstBuffer)
	{
		shadowMaterial = new StandardShadowMaterial;
		shadowMaterial->setUniformBuffer(shadowConstBuffer);
		shadowMaterial->LoadFromFilename(device, physicalDevice, shadowCommandPool, ShadowQueue, "standardShadow"  + objectName + "material");
		shadowMaterial->setShaderPaths("shaders/standardShadowMap.vert.spv", "shaders/standardShadowMap.frag.spv", "", "", "", "");

		
		shadowMaterial->createDescriptorSet();
		shadowMaterial->connectRenderPass(standardShadowRenderPass);
		shadowMaterial->createGraphicsPipeline(extent, offset);

	}

	std::vector<Geo*> geos;
	std::vector<Material*> materials;
	std::string objectName;

	StandardShadowMaterial* shadowMaterial;

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkCommandPool commandPool;
	VkQueue queue;

	std::string path;

	int materialOffset;

	bool bRoll;
	bool UflipCorrection;

	float rollSpeed;

	BoundingBox AABB;

private:


};
