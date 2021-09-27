cmake_minimum_required(VERSION 3.8)

project("MarCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	MarCore STATIC
	"src/MarCore.cpp"
	"src/Memory.cpp"
	"src/Assembler.cpp"
	"src/AssemblerOutput.cpp"
	"src/Interpreter.cpp"
)

target_include_directories(
	MarCore PUBLIC
	"include"
)