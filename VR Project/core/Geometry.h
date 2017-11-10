#pragma once
#include "../core/Common.h"
#include "../core/Vertex.h"
#include "tiny_obj_loader.h"
#include "Asset.h"

/*
struct BoundingBox
{
	glm::vec3 Center;            // Center of the box.
	glm::vec3 Extents;           // Distance from the center to each side.
};
*/

class Geo : public Asset
{
public:
	Geo();
	~Geo();

	void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string path);

	void loadGeometryFromFile(std::string path);



	void createVertexBuffer();
	void createIndexBuffer();

	

	virtual void cleanUp();

	std::vector<glm::vec3> Vpositions;
	std::vector<glm::vec3> Vnormals;
	std::vector<glm::vec2> Vuvs;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
private:

	
};

