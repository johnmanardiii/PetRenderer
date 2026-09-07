//
// Created by johnm on 8/12/2026.
//

#include "BabysFirstGraphicsPipeline.h"

#include "OffscreenRenderImage.h"
#include "ShaderLoader.h"
#include "Vector3D.h"

BabyFirstGraphicsPipelineInfo CreateBabysFirstGraphicsPipelineInfo(GpuContext &gpuContext,
                                                                   const OffscreenRenderImageInfo &
                                                                   offscreen_render_image_info)
{
    // create the layout for the pipeline
    VkPipelineLayoutCreateInfo pipeline_layout_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0
    };

    VkPipelineLayout graphicsPipelineLayout;
    vkchk(vkCreatePipelineLayout(gpuContext.device, &pipeline_layout_create_info, nullptr, &graphicsPipelineLayout));

    // create the shaders we are going to use (vert and frag for now)
    VkShaderModule vertexShaderModule;
    if (!LoadShader("../shaders/vertex_buffer_triangle/vertex_buffer_triangle.vert.spv", gpuContext.device,
                    &vertexShaderModule))
    {
        assert(false);
    }

    VkPipelineShaderStageCreateInfo vertexStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "main"
    };

    // fragment shader create info
    VkShaderModule fragmentShaderModule;
    if (!LoadShader("../shaders/vertex_buffer_triangle/vertex_buffer_triangle.frag.spv", gpuContext.device,
                    &fragmentShaderModule))
    {
        assert(false);
    }

    VkPipelineShaderStageCreateInfo fragmentStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShaderModule,
        .pName = "main"
    };

    // create the pipeline
    VkPipelineShaderStageCreateInfo stageInfos[2] =
    {
        vertexStageCreateInfo,
        fragmentStageCreateInfo
    };

    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_description =
    {
        .location = 0, // #JOHN - still not fully sure how to appropriatly connect this to slang
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, x)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attribute_description
    };

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
        // other things can go here but I don't care about them right now, except for this linewidth thing
        .lineWidth = 1.0f
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
        .depthTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // for the only color attachment
    VkPipelineColorBlendAttachmentState color_blend_attachment_state =
    {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state
    };

    VkFormat colorAttachmentFormat = offscreen_render_image_info.offscreenImageFormat;

    VkPipelineRenderingCreateInfo rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorAttachmentFormat,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_create_info,
        .flags = 0,
        .stageCount = 2,
        .pStages = stageInfos,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = nullptr, // don't think I need this if I don't have tesselation.
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamicStates,
        .layout = graphicsPipelineLayout,
    };

    VkPipeline babyPipeline = VK_NULL_HANDLE;

    // CREATE PIPELINE HERE
    vkchk(vkCreateGraphicsPipelines(gpuContext.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &babyPipeline));

    // now we can delete the shader modules we used to create it
    vkDestroyShaderModule(gpuContext.device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(gpuContext.device, fragmentShaderModule, nullptr);

    return {
        .graphicsPipelineLayout = graphicsPipelineLayout,
        .graphicsPipeline = babyPipeline
    };
}

void DestroyBabysFirstGraphicsPipelineInfo(GpuContext &gpuContext, BabyFirstGraphicsPipelineInfo &pipeline_info)
{
    vkDestroyPipeline(gpuContext.device, pipeline_info.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(gpuContext.device, pipeline_info.graphicsPipelineLayout, nullptr);
}

StuffINeedToTrackForAVertexBuffer CreateBasicVertexBuffer(VmaAllocator vmaAllocator)
{
    Vector3D positions[3] = {{-.5f, .5f, 0.0f}, {.5f, .5f, 0.0f}, {0.0f, -.5f, 0.0f}};

    // was reading over this: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_allocation_create_info.html
    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBufferCreateInfo buffer_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(positions),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // #JOHN probably want some other ones I can't think of rn
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,

    };

    StuffINeedToTrackForAVertexBuffer returnVal;

    vkchk(vmaCreateBuffer(vmaAllocator, &buffer_create_info, &allocCreateInfo, &returnVal.vertexBuffer,
                          &returnVal.vertexBufferAlloc, &returnVal.vertexBufferAllocInfo));

    memcpy(returnVal.vertexBufferAllocInfo.pMappedData, &positions, sizeof(positions));

    // now the vertex buffer contains the data...

    return returnVal;
}

void DestroyBasicVertexBuffer(VmaAllocator vmaAllocator, StuffINeedToTrackForAVertexBuffer &vertexInfo)
{
    vmaDestroyBuffer(vmaAllocator, vertexInfo.vertexBuffer, vertexInfo.vertexBufferAlloc);
}