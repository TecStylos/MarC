cmake_minimum_required(VERSION 3.8)

project("BUILD ALL")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory("vendor/PluS")
add_subdirectory("stdlib/std")
add_subdirectory("MarCore")
add_subdirectory("MarCmd")
