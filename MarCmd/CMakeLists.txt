cmake_minimum_required(VERSION 3.8)

project("MarCmd")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	MarCmd
	"src/main.cpp"
)

target_include_directories(
	MarCmd PUBLIC 
	"${CMAKE_SOURCE_DIR}/MarCore/include"
)

target_link_libraries(
	MarCmd PUBLIC
	MarCore
)