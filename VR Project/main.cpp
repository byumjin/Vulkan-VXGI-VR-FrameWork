#include "core/VulkanApp.h"

int main()
{
	VulkanApp vulkanApp;
	
	try
	{
		vulkanApp.run();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}	

	return EXIT_SUCCESS;
}
