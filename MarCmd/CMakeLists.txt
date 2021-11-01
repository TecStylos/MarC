cmake_minimum_required(VERSION 3.8)

project("MarCmd")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	MarCmd
	
	"src/EntryPoint.cpp"
	"include/CmdArgParser.h"
	"src/MarCmdInterpreter.cpp"
 "include/MarCmdFlags.h" "include/MarCmdLiveAsmInterpreter.h" "src/MarCmdLiveAsmInterpreter.cpp")

target_include_directories(
	MarCmd PUBLIC 
	"${CMAKE_SOURCE_DIR}/MarCmd/include"
	"${CMAKE_SOURCE_DIR}/MarCore/include"
)

target_link_libraries(
	MarCmd PUBLIC
	MarCore
)

if (MSVC) 
  target_link_options(MarCmd PRIVATE $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
endif()