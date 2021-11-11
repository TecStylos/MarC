cmake_minimum_required(VERSION 3.8)

project("std")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	EXTENSION_STD SHARED 
	"std.cpp"
)

set_target_properties(
	EXTENSION_STD PROPERTIES OUTPUT_NAME "std"
)
set_target_properties(
	EXTENSION_STD PROPERTIES PREFIX ""
)
set_target_properties(
	EXTENSION_STD PROPERTIES DEBUG_POSTFIX "-dbg"
)
set_target_properties(
	EXTENSION_STD PROPERTIES RELWITHDEBINFO_POSTFIX "-rel"
)
set_target_properties(
	EXTENSION_STD PROPERTIES RELEASE_POSTFIX "-dist"
)

target_include_directories(
	EXTENSION_STD PUBLIC 
	"../../PluS/include"
)

target_link_libraries(
   EXTENSION_STD PUBLIC 
   MarCore
   PluS
)

target_compile_definitions(
	EXTENSION_STD PUBLIC
	${PLUS_COMPILE_DEFINITIONS}
	"PLUS_BUILD_PLUGIN"
)