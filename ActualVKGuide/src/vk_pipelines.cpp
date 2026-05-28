#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>

// #JOHN when i do this... gonna need some testing and cross platform stuff fr

bool vkutil::load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
	// open file with cursor at end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	// find size of file by looking up location of cursor (at end), giving size in byte
	size_t fileSize = (size_t)file.tellg();

	// spirv expects buffer to be on uint32, so make it big enough for entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t)); // is this the right size or potentially one too small?
	
	// move to beginning
	file.seekg(0);

	// load file into buffer
	file.read((char*)buffer.data(), fileSize);

	file.close();

	// create a new shader module, using buffer we loaded

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	//codeSize is in bytes, so multiply ints in buffer by size of int to know size of buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();	// Q: how is this NOT a return to data on stack...

	// check creation went well
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}
