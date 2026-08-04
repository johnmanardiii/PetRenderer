target_sources(PetRenderer PRIVATE
        main.cpp
        VulkanInstanceBuilder.cpp
        VulkanEngine.cpp
        VulkanPhysicalDeviceSelector.cpp
        VulkanLogicalDeviceCreator.cpp
        VulkanSwapchainResources.cpp
        FrameInFlightStructures.cpp
        OffscreenRenderImage.cpp
        DescriptorAllocator.cpp
        ShaderLoader.cpp
        CreatePipeline.cpp
        VulkanUtil.cpp
        Platform.cpp
        GpuContext.cpp
)

target_include_directories(PetRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(PetRenderer PRIVATE VK_NO_PROTOTYPES)

target_link_libraries(PetRenderer PRIVATE SDL3::SDL3 fmt::fmt volk::volk)