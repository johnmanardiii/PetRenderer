cmake_minimum_required(VERSION 3.31)

project(PetRenderer
        VERSION 1.0
        LANGUAGES CXX
)

# (should probably do this for other platforms) turn off exceptions https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=msvc-170
if (MSVC)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(
        /EHs-c- # no exception unwinding
        /GR- # no RTTI
    )
    add_compile_definitions(_HAS_EXCEPTIONS=0)
endif()

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(PetRenderer
        src/Matrix4D.h
        src/CustomMath.h
        src/Application.cpp
        src/Application.h
        src/Model.cpp
        src/Model.h)

add_subdirectory(third_party)

add_subdirectory(src)
