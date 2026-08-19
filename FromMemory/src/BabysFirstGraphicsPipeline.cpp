//
// Created by johnm on 8/12/2026.
//

#include "BabysFirstGraphicsPipeline.h"

#include "ShaderLoader.h"

BabyFirstGraphicsPipelineInfo CreateBabysFirstGraphicsPipelineInfo(GpuContext &gpuContext)
{
    // create the shaders we are going to use (vert and frag for now)

    VkShaderModule vertexShaderModule;
    if (!LoadShader("../shaders/hardcoded_triangle/hardcoded_triangle.vert.spv", gpuContext.device,
                    &vertexShaderModule))
    {
        assert(false);
    }

    VkPipelineShaderStageCreateInfo vertexStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "vertexMain"
    };

    // fragment shader create info
    VkShaderModule fragmentShaderModule;
    if (!LoadShader("../shaders/hardcoded_triangle/hardcoded_triangle.frag.spv", gpuContext.device,
                    &fragmentShaderModule))
    {
        assert(false);
    }

    VkPipelineShaderStageCreateInfo fragmentStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = fragmentShaderModule,
        .pName = "fragmentMain"
    };

    // create the pipeline
    VkPipelineShaderStageCreateInfo stageInfos[2] =
    {
        vertexStageCreateInfo,
        fragmentStageCreateInfo
    };

    // vertex input state create info... I don't think i'll describe anything here just yet and pray it is something I can put off for later
    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr, // going to use dynamic state for viewport size
        .scissorCount = 1,
        .pScissors = nullptr // guess I am also going to make this dynamic
    };

    VkDynamicState viewportDynamicStates[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStates = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2, // for now
        .pDynamicStates = viewportDynamicStates
    };

    // up next, rasterization state and a significant amount of trips through the graphics pipeline with notes

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // forgot what is standard here
        .depthBiasEnable = VK_FALSE, // don't want this for now
        // other things can go here but I don't care about them right now
    };

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        // UHHH AND THEN THE REST OF THESE i DON'T REALLY GET HOPEFULLY ITS OK TO LEAVE 0/ FALSE
    };

    // ok I am officially too tired for this but it doesn't seem to hard if I don't care about either right now.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = 0,
        .stageCount = 2,
        .pStages = stageInfos,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = nullptr, // don't think I need this if I don't have tesselation.
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDynamicState = &dynamicStates
    };

    VkPipeline babyPipeline = VK_NULL_HANDLE;

    // CREATE PIPELINE HERE
    vkchk(vkCreateGraphicsPipelines(gpuContext.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &babyPipeline));

    // now we can delete the shader modules we used to create it
    vkDestroyShaderModule(gpuContext.device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(gpuContext.device, fragmentShaderModule, nullptr);

    return BabyFirstGraphicsPipelineInfo();
}

void DestroyBabysFirstGraphicsPipelineInfo()
{
}