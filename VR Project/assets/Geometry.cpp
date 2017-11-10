#include "Geometry.h"

Geo::Geo()
{
}

Geo::~Geo()
{
	cleanUp();
}

void Geo::LoadFromFilename(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	commandPool = commandPoolParam;
	queue = queueParam;

	path = pathParam;

	loadGeometryFromFile(path);

	createTBN();

	createVertexBuffer();
	createIndexBuffer();
}

void Geo::createTBN()
{
	for (size_t i = 0; i < numTriangles; i++)
	{
		glm::vec3 localPos[3];
		glm::vec2 uv[3];

		localPos[0] = vertices[3 * i].positions;
		localPos[1] = vertices[3 * i + 1].positions;
		localPos[2] = vertices[3 * i + 2].positions;

		uv[0] = vertices[3 * i].texcoords;
		uv[1] = vertices[3 * i + 1].texcoords;
		uv[2] = vertices[3 * i + 2].texcoords;



		float u0 = uv[1].x - uv[0].x;
		float u1 = uv[2].x - uv[0].x;

		float v0 = uv[1].y - uv[0].y;
		float v1 = uv[2].y - uv[0].y;

		float dino = u0 * v1 - v0 * u1;

		glm::vec3 Pos1 = localPos[1] - localPos[0];
		glm::vec3 Pos2 = localPos[2] - localPos[0];

		glm::vec2 UV1 = uv[1] - uv[0];
		glm::vec2 UV2 = uv[2] - uv[0];

		glm::vec3 tan;
		glm::vec3 bit;
		glm::vec3 nor;


		if (dino != 0.0f)
		{
			tan = glm::normalize((UV2.y * Pos1 - UV1.y * Pos2) / dino);
			bit = glm::normalize((Pos2 - UV2.x * tan) / UV2.y);
			nor = glm::normalize(glm::cross(tan, bit));
		}
		else
		{

			tan = glm::vec3(1.0f, 0.0f, 0.0f);
			bit = glm::vec3(0.0f, 1.0f, 0.0f);
			nor = glm::vec3(0.0f, 0.0f, 1.0f);
			//bit = glm::normalize(glm::cross(nor, tan));
			//tan = glm::normalize(glm::cross(bit, nor));
		}

		// Calculate handedness
		glm::vec3 fFaceNormal = glm::normalize(glm::cross(localPos[1] - localPos[0], localPos[2] - localPos[1]));

		//U flip
		if (glm::dot(nor, fFaceNormal) < 0.0f)
		{
			tan = -(tan);
		}

		glm::vec3 Nor[3];

		Nor[0] = vertices[3 * i].normals;
		Nor[1] = vertices[3 * i + 1].normals;
		Nor[2] = vertices[3 * i + 2].normals;

		glm::vec3 Tan[3];

		Tan[0] = tan;
		Tan[1] = tan;
		Tan[2] = tan;

		glm::vec3 BiTan[3];

		BiTan[0] = bit;
		BiTan[1] = bit;
		BiTan[2] = bit;

		if (Nor[0] != glm::vec3(0.0f))
		{
			nor = Nor[0];
			BiTan[0] = glm::normalize(glm::cross(nor, Tan[0]));
			Tan[0] = glm::normalize(glm::cross(BiTan[0], nor));
		}

		if (Nor[1] != glm::vec3(0.0f))
		{
			nor = Nor[1];
			BiTan[1] = glm::normalize(glm::cross(nor, Tan[1]));
			Tan[1] = glm::normalize(glm::cross(BiTan[1], nor));
		}

		if (Nor[2] != glm::vec3(0.0f))
		{
			nor = Nor[2];
			BiTan[2] = glm::normalize(glm::cross(nor, Tan[2]));
			Tan[2] = glm::normalize(glm::cross(BiTan[2], nor));
		}

		vertices[3 * i].tangents = Tan[0];
		vertices[3 * i + 1].tangents = Tan[1];
		vertices[3 * i + 2].tangents = Tan[2];

		vertices[3 * i].bitangents = BiTan[0];
		vertices[3 * i + 1].bitangents = BiTan[1];
		vertices[3 * i + 2].bitangents = BiTan[2];
	}
}

void Geo::loadGeometryFromFile(std::string path)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	
	std::string errors = tinyobj::LoadObj(shapes, materials, path.c_str());	

	if (errors.size() != 0)
	{
		throw std::runtime_error("failed to load Object!");
	}
	else
	{
		int min_idx = 0;
		//Read the information from the vector of shape_ts
		for (unsigned int i = 0; i < shapes.size(); i++)
		{
			indices = shapes[i].mesh.indices;
			std::vector<float> &positions = shapes[i].mesh.positions;
			std::vector<float> &normals = shapes[i].mesh.normals;
			std::vector<float> &uvs = shapes[i].mesh.texcoords;

			for (unsigned int j = 0; j < positions.size() / 3; j++)
			{
				Vpositions.push_back(glm::vec3(positions[j * 3], positions[j * 3 + 1], positions[j * 3 + 2]));
			}

			for (unsigned int j = 0; j < normals.size() / 3; j++)
			{
				Vnormals.push_back(glm::vec3(normals[j * 3], normals[j * 3 + 1], normals[j * 3 + 2]));
			}

			for (unsigned int j = 0; j < uvs.size() / 2; j++)
			{
				Vuvs.push_back(glm::vec2(uvs[j * 2], 1.0 - uvs[j * 2 + 1]));
			}


			numVetices = (unsigned int)positions.size() / 3;
			numTriangles = numVetices / 3;
			for (unsigned int j = 0; j < numVetices; j++)
			{
				Vertex tempVertexInfo;
				tempVertexInfo.positions = Vpositions[j];
				tempVertexInfo.colors = glm::vec3(1.0f);

				if (normals.size() == 0)
					tempVertexInfo.normals = glm::vec3(0.0f);
				else
					tempVertexInfo.normals = Vnormals[j];
				
				if (uvs.size() == 0)
					tempVertexInfo.texcoords = glm::vec2(0.0f, 0.0f);
				else
					tempVertexInfo.texcoords = Vuvs[j];

				vertices.push_back(tempVertexInfo);
			}

			return;
		}
	}
}


void Geo::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Geo::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(unsigned int) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Geo::cleanUp()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}