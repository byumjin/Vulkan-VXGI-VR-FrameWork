#include "Object.h"

Object::Object():materialOffset(0), bRoll(false), UflipCorrection(false)
{
}

void Object::init(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam, int materialOffsetParam,
	bool needUflipCorrection)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	commandPool = commandPoolParam;
	queue = queueParam;
	path = pathParam;
	
	objectName = pathParam;

	materialOffset = materialOffsetParam;

	UflipCorrection = needUflipCorrection;

	loadObjectFromFile(path);
}

void Object::loadObjectFromFile(std::string path)
{
	std::vector<tinyobj::shape_t> _shapes;
	std::vector<tinyobj::material_t> _materials;

	std::string errors = tinyobj::LoadObj(_shapes, _materials, path.c_str());

	//geos.resize(shapes.size());
	materials.resize(_shapes.size());

	if (errors.size() != 0)
	{
		throw std::runtime_error("failed to load Object!");
	}
	else
	{
		int min_idx = 0;
		//Read the information from the vector of shape_ts
		for (unsigned int i = 0; i < _shapes.size(); i++)
		{
			Geo* tempGeo = new Geo;
			tempGeo->init(device, physicalDevice, commandPool, queue, path, UflipCorrection);
			tempGeo->setGeometry(_shapes[i]);
			tempGeo->createTBN();
			tempGeo->createVertexBuffer();
			tempGeo->createIndexBuffer();
			geos.push_back(tempGeo);
		}

		setAABB();
	}
}

void Object::initiation(std::string objectNameParam, Geo* geoParam)
{
	objectName = objectNameParam;
	//geo = geoParam;
}


void Object::connectMaterial(Material* mat, unsigned int matIndex)
{
	int materialIndex = matIndex + materialOffset;

	if (materialIndex < 0)
		return;
	else
		materials[materialIndex] = mat;
}
