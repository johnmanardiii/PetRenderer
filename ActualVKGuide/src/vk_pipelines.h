#pragma once 
#include <vk_types.h> 

namespace vkutil {
	bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
};

class PipelineBuilder
{
public:
	// ok so this is probably used for frag / vert and count == num shader stages
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipelineRenderingCreateInfo _renderInfo;
	VkFormat _colorAttachmentFormat;

	PipelineBuilder() { clear(); }
	void clear();

	VkPipeline build_pipeline(VkDevice device);

	void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	void set_input_topology(VkPrimitiveTopology topology);
	void set_polygon_mode(VkPolygonMode mode);
	void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void set_multisampling_none();
};