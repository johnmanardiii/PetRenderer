cmake_minimum_required(VERSION 3.31)

project(PetRenderer
        VERSION 1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(third_party)

#JOHN - clion puts it here when i make a new file... but maybe I want it in the Src cmakelists? in a different typa
# statement than in the add excutable?
add_executable(PetRenderer)

add_subdirectory(src)
