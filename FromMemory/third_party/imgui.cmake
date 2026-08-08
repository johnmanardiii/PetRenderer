target_sources(PetRenderer PRIVATE
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_vulkan.cpp
)

target_include_directories(PetRenderer PRIVATE imgui imgui/backends)

target_compile_definitions(PetRenderer PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)