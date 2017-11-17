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
		for (size_t i = 0; i < geos.size(); i++)
		{
			delete geos[i];
		}
	}

	void init(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam, int materialOffsetParam, bool needUflipCorrection);

	void loadObjectFromFile(std::string path);

	void initiation(std::string objectNameParam, Geo* geoParam);

	void connectMaterial(Material* mat, unsigned int matIndex);

	std::vector<Geo*> geos;
	std::vector<Material*> materials;
	std::string objectName;


	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkCommandPool commandPool;
	VkQueue queue;

	std::string path;

	int materialOffset;

	bool bRoll;
	bool UflipCorrection;

private:


};
