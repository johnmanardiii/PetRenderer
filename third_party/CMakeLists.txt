include (FetchContent)

find_package(Vulkan REQUIRED)

# switched to sdl from glfw for multiplatform windowing support.
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.8
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        12.1.0 # compatible with clang 21 for android
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG 1.4.350
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(volk)

include(imgui.cmake)
include(vma.cmake)
include(cgltf.cmake)

# silly android ndk... just trying to get this to compile so i copied it over
if (ANDROID)
        target_include_directories(PetRenderer PRIVATE vulkan_util)
endif()