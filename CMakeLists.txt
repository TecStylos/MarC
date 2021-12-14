cmake_minimum_required(VERSION 3.8)

project("BUILD ALL")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_BUILD_TYPE_RELEASE)
    
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

add_subdirectory("vendor/PluS/PluS")
add_subdirectory("stdlib/std")
add_subdirectory("MarCore")
add_subdirectory("MarCmd")
