add_library(vma STATIC vma/vma_impl.cpp)
target_include_directories(vma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vma)
target_link_libraries(vma PUBLIC volk::volk)
target_compile_definitions(vma PUBLIC VK_NO_PROTOTYPES)