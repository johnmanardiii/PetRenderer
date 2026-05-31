target_sources(PetRenderer PRIVATE
        main.cpp
        VulkanInstanceBuilder.cpp
        VulkanEngine.cpp
        VulkanPhysicalDeviceSelector.cpp
        VulkanLogicalDeviceCreator.cpp
        VulkanSwapchainResources.cpp
        VulkanCommandBufferCreator.cpp
)

target_include_directories(PetRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(PetRenderer PRIVATE VK_NO_PROTOTYPES)

target_link_libraries(PetRenderer PRIVATE SDL3::SDL3 fmt::fmt volk::volk)