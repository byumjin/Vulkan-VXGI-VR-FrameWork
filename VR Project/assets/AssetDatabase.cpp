#include "AssetDatabase.h"

AssetDatabase * AssetDatabase::instance = nullptr;
VkDevice AssetDatabase::device = nullptr;
VkPhysicalDevice AssetDatabase::physicalDevice = nullptr;
VkCommandPool AssetDatabase::commandPool = NULL;
VkQueue AssetDatabase::queue = nullptr;

//std::vector<WCHAR*> AssetDatabase::m_ListofGeometry;
//std::vector<WCHAR*> AssetDatabase::m_ListofTexture;

AssetDatabase::AssetDatabase()
{
}

AssetDatabase *AssetDatabase::GetInstance()
{
	if (instance == nullptr)
		instance = new AssetDatabase();

	return instance;
}
