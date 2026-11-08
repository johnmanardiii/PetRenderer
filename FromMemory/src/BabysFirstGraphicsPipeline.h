//
// Created by johnm on 8/12/2026.
//

#ifndef BABYSFIRSTGRAPHICSPIPELINE_H
#define BABYSFIRSTGRAPHICSPIPELINE_H
#include "GpuContext.h"
#include "vk_mem_alloc.h"


/*
 *  Basically a throwaway file just to get a hardcoded triangle to the screen
 *
 *  meant for getting the practice of setting up what it takes to make a graphics pipeline
 *  Trying not to take myself too seriously and just get a triangle on screen here
 */

struct OffscreenRenderImageInfo;

struct BabyFirstGraphicsPipelineInfo
{
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
};

BabyFirstGraphicsPipelineInfo CreateBabysFirstGraphicsPipelineInfo(GpuContext& gpuContext, const OffscreenRenderImageInfo& offscreen_render_image_info);

void DestroyBabysFirstGraphicsPipelineInfo(GpuContext& gpuContext, BabyFirstGraphicsPipelineInfo& pipeline_info);

#endif //BABYSFIRSTGRAPHICSPIPELINE_H
