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


void PipelineBuilder::clear()
{
	// clear all structs to 0 but with their s type filled in as valid
	_inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	_colorBlendAttachment = {};

	_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	_pipelineLayout = {};

	_depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	_renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	_shaderStages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device)
{
	// make viewport state from stored viewport and scissor, currently only one viewport / scissor
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// dummy color blending, so just no blending right now, we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	// clear vertex input state create info (we don't need it) or something
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// ^^ no idea what this actually means

	// start filling in VkGraphicsPipelineCreateInfo
	// build pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// WHAT IS RENDER INFO AGAIN?
	pipelineInfo.pNext = &_renderInfo;
	pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;

	// set up dynamic state
	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2; // corresponding to the above
	pipelineInfo.pDynamicState = &dynamicInfo;

	// call create function
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
	{
		fmt::println("failed to create graphics pipeline");
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}

void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
	_shaderStages.clear();
	_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
	_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
{
	_inputAssembly.topology = topology;

	// leave on false, but what is primitive restart?
	_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
	_rasterizer.polygonMode = mode;
	_rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	_rasterizer.cullMode = cullMode;
	_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::set_multisampling_none()
{
	_multisampling.sampleShadingEnable = VK_FALSE;
	_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_multisampling.minSampleShading = 1.0f;
	_multisampling.pSampleMask = nullptr;

	// alpha to coverage? what that mean
	_multisampling.alphaToCoverageEnable = VK_FALSE;
	_multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::disable_blending()
{
	// default write mask
	_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// no blending
	_colorBlendAttachment.blendEnable = VK_FALSE;
}


void PipelineBuilder::set_color_attachment_format(VkFormat format)
{
	_colorAttachmentFormat = format;
	// connect to render info structure
	_renderInfo.colorAttachmentCount = 1;
	_renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}


void PipelineBuilder::set_depth_format(VkFormat format)
{
	_renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::disable_depthtest()
{
	_depthStencil.depthTestEnable = VK_FALSE;
	_depthStencil.depthWriteEnable = VK_FALSE;
	_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable = VK_FALSE;
	_depthStencil.front = {};
	_depthStencil.back = {};
	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}
