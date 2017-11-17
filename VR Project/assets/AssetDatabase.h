#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <vector>

#include "../actors/Object.h"

//ObjectList
static std::vector<Object*> objectManager;
static std::vector<Material*> materialManager;

struct AssetData_GeoListInfo
{
	std::string GeometryPath;
};

struct AssetData_TextureListInfo
{
	std::string TexturePath;
};

struct AssetData_MaterialListInfo
{
	std::string MaterialName;
};

// Basically all heavy things like shaders, meshes or textures
// are cached in memory. They can be instanced independently if
// no caching is desired.
class AssetDatabase
{
private:
	static AssetDatabase * instance;

	static VkPhysicalDevice physicalDevice;
	static VkDevice device;
	static VkCommandPool commandPool;
	static VkQueue queue;
	

	std::unordered_map<std::type_index, std::unordered_map<std::string, Asset*>> assetMap;
	AssetDatabase();

public:

	//GeoList
	std::vector<std::string> geoList;

	//TextureList
	std::vector<std::string> textureList;

	//MaterialList
	std::vector<std::string> materialList;

	~AssetDatabase()
	{
		
	}

	void cleanUp()
	{
		for (uint32_t i = 0; i < geoList.size(); i++)
		{
			Geo* pGeo = FindAsset<Geo>(geoList[i]);
			delete pGeo;
		}
		
		for (uint32_t i = 0; i < textureList.size(); i++)
		{
			Texture* pTex = FindAsset<Texture>(textureList[i]);
			delete pTex;
		}
		
		for (uint32_t i = 0; i < materialList.size(); i++)
		{
			Material* pMaterial = FindAsset<Material>(materialList[i]);
			delete pMaterial;
		}

		for (uint32_t i = 0; i < materialManager.size(); i++)
		{			
			delete materialManager[i];
		}
		
		assetMap.clear();

		geoList.clear();
		textureList.clear();
		materialList.clear();

		materialManager.clear();
	}

	static void SetDevice(VkDevice pDevice, VkPhysicalDevice pphysicalDevice, VkCommandPool pcommandPool, VkQueue pqueue)
	{
		device = pDevice;
		physicalDevice = pphysicalDevice;
		commandPool = pcommandPool;
		queue = pqueue;
	}

	static Material* LoadMaterial(std::string materialName)
	{
		for (size_t i = 0; i < materialManager.size(); i++)
		{
			if (materialManager[i]->path == materialName)
			{
				return materialManager[i];
			}
		}

		return NULL;

	}

	static AssetDatabase * GetInstance();

	template <class T>
	T* LoadAsset(std::string id)
	{
		T * t = FindAsset<T>(id);

		if (t != nullptr)
			return t;

		t = new T();
		t->LoadFromFilename(device, physicalDevice, commandPool, queue, id);
		assetMap[typeid(T)][id] = t;
		return t;
	}


	template<class T>
	T * FindAsset(std::string id)
	{
		std::unordered_map<std::string, Asset*>& idMap = assetMap[typeid(T)];

		if (idMap.find(id) != idMap.end())
			return dynamic_cast<T*>(idMap[id]);

		return nullptr;
	}

};