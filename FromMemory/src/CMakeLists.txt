target_sources(PetRenderer PRIVATE
    main.cpp
    vulkan_util_pr.h
)

target_include_directories(PetRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# #JOHN does this extend to like... the actual shipped application or the SDK? I feel like I don't ship the
# SDK with my program but I also don't fully understand "including" vulkan into cpp.
# not entirely sure why yet but I couldn't put this in the third party dir #JOHN
find_package(Vulkan REQUIRED)

target_link_libraries(PetRenderer PRIVATE glfw Vulkan::Vulkan fmt::fmt)