#include "Geometry.h"

Geo::Geo():UflipCorrection(false)
{
}

Geo::~Geo()
{
	cleanUp();
}

void Geo::init(VkDevice deviceParam, VkPhysicalDevice physicalDeviceParam, VkCommandPool commandPoolParam, VkQueue queueParam, std::string pathParam, bool needUflipCorrection)
{
	device = deviceParam;
	physicalDevice = physicalDeviceParam;
	commandPool = commandPoolParam;
	queue = queueParam;

	path = pathParam;

	UflipCorrection = needUflipCorrection;
}

void Geo::setGeometry(tinyobj::shape_t &shape)
{
	this->indices = shape.mesh.indices;
	std::vector<float> &positions = shape.mesh.positions;
	std::vector<float> &normals = shape.mesh.normals;
	std::vector<float> &uvs = shape.mesh.texcoords;

	for (unsigned int j = 0; j < positions.size() / 3; j++)
	{
		this->Vpositions.push_back(glm::vec3(positions[j * 3], positions[j * 3 + 1], positions[j * 3 + 2]));
	}

	for (unsigned int j = 0; j < normals.size() / 3; j++)
	{
		this->Vnormals.push_back(glm::vec3(normals[j * 3], normals[j * 3 + 1], normals[j * 3 + 2]));
	}

	for (unsigned int j = 0; j < uvs.size() / 2; j++)
	{
		this->Vuvs.push_back(glm::vec2(uvs[j * 2], 1.0 - uvs[j * 2 + 1]));
	}
	
	this->numVetices = (unsigned int)this->Vpositions.size();

	glm::vec3 maxCorner = glm::vec3(-FLT_MAX);
	glm::vec3 minCorner = glm::vec3(FLT_MAX);;

	for (unsigned int j = 0; j < this->numVetices; j++)
	{
		Vertex tempVertexInfo;
		tempVertexInfo.positions = this->Vpositions[j];
		tempVertexInfo.colors = glm::vec3(0.0f);

		if (normals.size() == 0)
			tempVertexInfo.normals = glm::vec3(0.0f, 0.0f, 1.0f);
		else
			tempVertexInfo.normals = this->Vnormals[j];

		if (uvs.size() == 0)
			tempVertexInfo.texcoords = glm::vec2(0.0f, 0.0f);
		else
			tempVertexInfo.texcoords = this->Vuvs[j];
		
		tempVertexInfo.tangents = glm::vec3(0.0f);
		tempVertexInfo.bitangents = glm::vec3(0.0f);

		this->vertices.push_back(tempVertexInfo);

		maxCorner = glm::max(maxCorner, tempVertexInfo.positions);
		minCorner = glm::min(minCorner, tempVertexInfo.positions);
	}

	
	
	AABB.maxPt = maxCorner;
	AABB.minPt = minCorner;

	AABB.Center = (maxCorner + minCorner) * 0.5f;
	AABB.Extents = glm::abs(maxCorner - minCorner) * 0.5f;

	AABB.Corners[0] = AABB.Center + glm::vec3(-AABB.Extents.x, -AABB.Extents.y, -AABB.Extents.z);
	AABB.Corners[1] = AABB.Center + glm::vec3(AABB.Extents.x, -AABB.Extents.y, -AABB.Extents.z);
	AABB.Corners[2] = AABB.Center + glm::vec3(-AABB.Extents.x, AABB.Extents.y, -AABB.Extents.z);
	AABB.Corners[3] = AABB.Center + glm::vec3(-AABB.Extents.x, -AABB.Extents.y, AABB.Extents.z);

	AABB.Corners[4] = AABB.Center + glm::vec3(AABB.Extents.x, AABB.Extents.y, -AABB.Extents.z);
	AABB.Corners[5] = AABB.Center + glm::vec3(AABB.Extents.x, -AABB.Extents.y, AABB.Extents.z);
	AABB.Corners[6] = AABB.Center + glm::vec3(-AABB.Extents.x, AABB.Extents.y, AABB.Extents.z);
	AABB.Corners[7] = AABB.Center + glm::vec3(AABB.Extents.x, AABB.Extents.y, AABB.Extents.z);
	

	this->numTriangles = (unsigned int)this->indices.size() / 3;
}

void Geo::createTBN()
{
	std::pair<int, int> tempPair;
	tempPair.first = -1;
	tempPair.second = -1;
	handness.resize(vertices.size(), tempPair);

	for (size_t i = 0; i < numTriangles; i++)
	{
		glm::vec3 localPos[3];
		glm::vec2 uv[3];

		unsigned int index01 = indices[3 * i];
		unsigned int index02 = indices[3 * i + 1];
		unsigned int index03 = indices[3 * i + 2];

		localPos[0] = vertices[index01].positions;
		localPos[1] = vertices[index02].positions;
		localPos[2] = vertices[index03].positions;

		uv[0] = vertices[index01].texcoords;
		uv[1] = vertices[index02].texcoords;
		uv[2] = vertices[index03].texcoords;



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

			// Calculate handedness
			glm::vec3 fFaceNormal = glm::normalize(glm::cross(Pos2, Pos1));

			//U flip
			if (glm::dot(nor, fFaceNormal) < 0.0f)
			{
				tan = -(tan);

				if (handness[index01].first == -1)
				{
					handness[index01].first = 0;
				}

				if (handness[index02].first == -1)
				{
					handness[index02].first = 0;
				}

				if (handness[index03].first == -1)
				{
					handness[index03].first = 0;
				}

				if (handness[index01].first == 1)
				{
					//없으면 생성
					if (handness[index01].second == -1)
					{
						vertices.push_back(vertices[index01]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 0;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index01].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i] = handness[index01].second;
					index01 = indices[3 * i];

					vertices[index01].colors.y = 1.0f;
				}

				if (handness[index02].first == 1)
				{
					//없으면 생성
					if (handness[index02].second == -1)
					{
						vertices.push_back(vertices[index02]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 0;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index02].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i + 1] = handness[index02].second;
					index02 = indices[3 * i + 1];

					vertices[index02].colors.y = 1.0f;
				}

				if (handness[index03].first == 1)
				{
					//없으면 생성
					if (handness[index03].second == -1)
					{
						vertices.push_back(vertices[index03]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 0;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index03].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i + 2] = handness[index03].second;
					index03 = indices[3 * i + 2];

					vertices[index03].colors.y = 1.0f;
				}

				if (vertices[index01].colors.y == 1.0f || vertices[index02].colors.y == 1.0f || vertices[index03].colors.y == 1.0f)
				{
					vertices[index01].colors.y = 1.0f;
					vertices[index02].colors.y = 1.0f;
					vertices[index03].colors.y = 1.0f;
				}

				vertices[index01].colors.x = 1.0f;
				vertices[index02].colors.x = 1.0f;
				vertices[index03].colors.x = 1.0f;
			}
			else
			{
				if (handness[index01].first == -1)
				{
					handness[index01].first = 1;
				}

				if (handness[index02].first == -1)
				{
					handness[index02].first = 1;
				}

				if (handness[index03].first == -1)
				{
					handness[index03].first = 1;
				}

				if (handness[index01].first == 0)
				{
					//없으면 생성
					if (handness[index01].second == -1)
					{
						vertices.push_back(vertices[index01]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 1;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index01].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i] = handness[index01].second;
					index01 = indices[3 * i];
					vertices[index01].colors.z = 1.0f;
				}

				if (handness[index02].first == 0)
				{
					//없으면 생성
					if (handness[index02].second == -1)
					{
						vertices.push_back(vertices[index02]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 1;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index02].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i + 1] = handness[index02].second;
					index02 = indices[3 * i + 1];
					vertices[index02].colors.z = 1.0f;
				}

				if (handness[index03].first == 0)
				{
					//없으면 생성
					if (handness[index03].second == -1)
					{
						vertices.push_back(vertices[index03]);

						vertices[vertices.size() - 1].tangents = glm::vec3(0.0f);
						vertices[vertices.size() - 1].bitangents = glm::vec3(0.0f);

						std::pair<int, int> newHandness;
						newHandness.first = 1;
						newHandness.second = -1;
						handness.push_back(newHandness);

						handness[index03].second = (int)vertices.size() - 1;
					}

					//connect new indice
					indices[3 * i + 2] = handness[index03].second;
					index03 = indices[3 * i + 2];
					vertices[index03].colors.z = 1.0f;
				}


				if (vertices[index01].colors.z == 1.0f || vertices[index02].colors.z == 1.0f || vertices[index03].colors.z == 1.0f)
				{
					vertices[index01].colors.z = 1.0f;
					vertices[index02].colors.z = 1.0f;
					vertices[index03].colors.z = 1.0f;
				}
			}

			if (glm::dot(tan, bit) == 1.0f)
			{
				continue;
			}
			
			
		}
		else
		{
			continue;
		}		

		

		glm::vec3 Nor[3];

		Nor[0] = vertices[index01].normals;
		Nor[1] = vertices[index02].normals;
		Nor[2] = vertices[index03].normals;

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

		vertices[index01].tangents += Tan[0];
		vertices[index02].tangents += Tan[1];
		vertices[index03].tangents += Tan[2];

		

		vertices[index01].bitangents += BiTan[0];
		vertices[index02].bitangents += BiTan[1];
		vertices[index03].bitangents += BiTan[2];
	}

	for (size_t i = 0; i < vertices.size(); i++)
	{
		
		if (glm::length(vertices[i].tangents) == 0.0f)
		{
			if (handness[i].first == 1)
				vertices[i].tangents = glm::vec3(1.0, 0.0, 0.0);
			else if (handness[i].first == 0)
				vertices[i].tangents = glm::vec3(-1.0, 0.0, 0.0);
			else
			{
				vertices[i].tangents = glm::vec3(1.0, 0.0, 0.0);
			}

		}

		if (glm::dot(vertices[i].normals, glm::vec3(1.0, 0.0, 0.0)) == 1.0f && glm::dot(vertices[i].normals, vertices[i].tangents) > 0.9999f)
		{
			if(handness[i].first == 1)
				vertices[i].tangents = glm::vec3(0.0, 0.0, -1.0);
			else
				vertices[i].tangents = glm::vec3(0.0, 0.0, 1.0);
		}
		else if (glm::dot(vertices[i].normals, glm::vec3(-1.0, 0.0, 0.0)) == 1.0f && glm::dot(vertices[i].normals, vertices[i].tangents) > 0.9999f)
		{
			if (handness[i].first == 1)
				vertices[i].tangents = glm::vec3(0.0, 0.0, 1.0);
			else
				vertices[i].tangents = glm::vec3(0.0, 0.0, -1.0);
		}

		
		

		vertices[i].tangents = normalize(vertices[i].tangents);

		if (UflipCorrection && vertices[i].colors.z == 1.0f)
			vertices[i].tangents = -vertices[i].tangents;

		vertices[i].bitangents = normalize(vertices[i].bitangents);

		

		glm::vec3 nor = vertices[i].normals;


		

		vertices[i].bitangents = glm::normalize(glm::cross(nor, vertices[i].tangents));
		vertices[i].tangents = glm::normalize(glm::cross(vertices[i].bitangents, nor));
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
				Vuvs.push_back(glm::vec2( glm::fract(uvs[j * 2]), 1.0 - glm::fract(uvs[j * 2 + 1])));
			}


			numVetices = (unsigned int)Vpositions.size();
			
			for (unsigned int j = 0; j < numVetices; j++)
			{
				Vertex tempVertexInfo;
				tempVertexInfo.positions = Vpositions[j];
				tempVertexInfo.colors = glm::vec3(0.0f);

				if (normals.size() == 0)
					tempVertexInfo.normals = glm::vec3(0.0f, 0.0f, 1.0f);
				else
					tempVertexInfo.normals = Vnormals[j];
				
				if (uvs.size() == 0)
					tempVertexInfo.texcoords = glm::vec2(0.0f, 0.0f);
				else
					tempVertexInfo.texcoords = Vuvs[j];

				if (glm::dot(tempVertexInfo.normals, tempVertexInfo.tangents) > 0.9999f)
					tempVertexInfo.tangents = glm::vec3(0.0f, 1.0f, 0.0f);
				else
					tempVertexInfo.tangents = glm::vec3(1.0f, 0.0f, 0.0f);

				tempVertexInfo.bitangents = glm::vec3(0.0f, 1.0f, 0.0f);

				vertices.push_back(tempVertexInfo);
			}

			numTriangles = (unsigned int)indices.size() / 3;

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