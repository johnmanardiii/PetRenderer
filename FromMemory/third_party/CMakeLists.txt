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
        GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281 # 10.2.1
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