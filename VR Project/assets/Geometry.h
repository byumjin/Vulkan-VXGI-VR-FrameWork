#pragma once

#include "../core/Vertex.h"
#include "tiny_obj_loader.h"
#include "Asset.h"

//#include "../actors/Camera.h"


struct BoundingBox
{
	glm::vec3 Center;            // Center of the box.
	glm::vec3 Extents;           // Distance from the center to each side.

	glm::vec3 minPt;
	glm::vec3 maxPt;

	glm::vec3 Corners[8];
};



class Geo : public Asset
{
public:
	Geo();
	~Geo();

	//virtual void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam);

	void loadGeometryFromFile(std::string path);

	void init(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam, bool needUflipCorrection);

	void setGeometry(tinyobj::shape_t &shape);

	void createVertexBuffer();
	void createIndexBuffer();

	virtual void cleanUp();

	std::vector<glm::vec3> Vpositions;
	std::vector<glm::vec3> Vnormals;
	std::vector<glm::vec2> Vuvs;
	std::vector<Vertex> vertices;
	std::vector<std::pair<int, int>> handness;

	std::vector<unsigned int> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	unsigned int numVetices;
	unsigned int numTriangles;

	void createTBN();
	
	bool intesect(glm::mat4 mvpMat)
	{
		for (int i = 0; i < 8; i++)
		{
			glm::vec4 temp = mvpMat * glm::vec4(AABB.Corners[i], 1.0f);
			temp /= temp.w;

			if (glm::abs(temp.x) < 1.0f && glm::abs(temp.y) < 1.0f && (temp.z > 0.0f &&  temp.z < 1.0f))
				return true;
		}

		return false;
	}
	BoundingBox AABB;

private:

	bool UflipCorrection;
	
};

class singleTriangular : public Geo
{
public:
	singleTriangular()
	{

	}
	~singleTriangular()
	{

	}

	virtual void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam)
	{
		device = deviceParam;
		physicalDevice = physicalDeviceParam;
		commandPool = commandPoolParam;
		queue = queueParam;

		path = pathParam;

		Vertex tempVertexInfo;
		tempVertexInfo.positions = glm::vec3(-1.0f, 3.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(0.0f, 2.0f);

		vertices.push_back(tempVertexInfo);
		indices.push_back(0);

		tempVertexInfo.positions = glm::vec3(-1.0f, -1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(tempVertexInfo);
		indices.push_back(1);

		tempVertexInfo.positions = glm::vec3(3.0f, -1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(2.0f, 0.0f);

		vertices.push_back(tempVertexInfo);
		indices.push_back(2);

		createVertexBuffer();
		createIndexBuffer();
	}

	void updateVertexBuffer(glm::mat4 InvViewProj)
	{
		std::vector<Vertex> newVertices;

		for (size_t i = 0; i < vertices.size(); i++)
		{
			glm::vec4 newPos = InvViewProj * glm::vec4(vertices[i].positions, 1.0f);
			newPos /= newPos.w;

			Vertex newVertex = vertices[i];
			newVertex.positions = newPos;

			newVertices.push_back(newVertex);
		}

		VkDeviceSize bufferSize = sizeof(Vertex) * newVertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, newVertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		//createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}


private:


};


class singleQuadral : public Geo
{
public:
	singleQuadral()
	{

	}
	~singleQuadral()
	{

	}

	virtual void LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam)
	{
		device = deviceParam;
		physicalDevice = physicalDeviceParam;
		commandPool = commandPoolParam;
		queue = queueParam;

		path = pathParam;

		Vertex tempVertexInfo;
		tempVertexInfo.positions = glm::vec3(-1.0f, 1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(0.0f, 1.0f);

		vertices.push_back(tempVertexInfo);
		

		tempVertexInfo.positions = glm::vec3(-1.0f, -1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(tempVertexInfo);
		

		tempVertexInfo.positions = glm::vec3(1.0f, -1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(1.0f, 0.0f);

		vertices.push_back(tempVertexInfo);
		

		tempVertexInfo.positions = glm::vec3(1.0f, 1.0f, 0.5f);
		tempVertexInfo.colors = glm::vec3(1.0f);
		tempVertexInfo.normals = glm::vec3(1.0f);
		tempVertexInfo.texcoords = glm::vec2(1.0f, 1.0f);

		vertices.push_back(tempVertexInfo);

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);

		indices.push_back(0);
		indices.push_back(2);
		indices.push_back(3);

		createVertexBuffer();
		createIndexBuffer();
	}

	void updateVertexBuffer(glm::mat4 InvViewProj)
	{
		std::vector<Vertex> newVertices;

		for (size_t i = 0; i < vertices.size(); i++)
		{
			glm::vec4 newPos = InvViewProj * glm::vec4(vertices[i].positions, 1.0f);
			newPos /= newPos.w;

			Vertex newVertex = vertices[i];
			newVertex.positions = newPos;

			newVertices.push_back(newVertex);
		}

		VkDeviceSize bufferSize = sizeof(Vertex) * newVertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, newVertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		//createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}


private:


};
