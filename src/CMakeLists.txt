target_sources(PetRenderer PRIVATE
        main.cpp
        VulkanEngine.cpp
        VulkanEngine.h
        VulkanSwapchainResources.cpp
        VulkanSwapchainResources.h
        FrameInFlightStructures.cpp
        FrameInFlightStructures.h
        OffscreenRenderImage.cpp
        OffscreenRenderImage.h
        DescriptorAllocator.cpp
        DescriptorAllocator.h
        ShaderLoader.cpp
        ShaderLoader.h
        VulkanUtil.cpp
        VulkanUtil.h
        Platform.cpp
        Platform.h
        GpuContext.cpp
        GpuContext.h
        VulkanInit.cpp
        VulkanInit.h

        # throwaway starter pipeline things
        CreatePipeline.cpp
        CreatePipeline.h

        BabysFirstGraphicsPipeline.cpp
        BabysFirstGraphicsPipeline.h

        Vector2D.h      # starting to think this should be a template...
        Vector3D.h
        Vector4D.h
        Matrix3D.h
        Matrix4D.h

        CustomMath.h
        Model.cpp
        Model.h

        FlyCamera.h
        FlyCamera.cpp

        CopyQueue.h
        CopyQueue.cpp

        ImmediateSubmit.h
        ImmediateSubmit.cpp
)

target_include_directories(PetRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(PetRenderer PRIVATE SDL3::SDL3 fmt::fmt volk::volk vma)