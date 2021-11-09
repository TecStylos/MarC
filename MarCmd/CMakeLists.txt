cmake_minimum_required(VERSION 3.8)

project("MarCmd")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	MarCmd
	"src/EntryPoint.cpp"
	"src/MarCmdInterpreter.cpp"
	"src/MarCmdLiveAsmInterpreter.cpp"
)

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

  add_custom_command(
    TARGET MarCmd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../bin/$<CONFIG>/vendor/PluS/PluS/PluS.dll"
    $<TARGET_FILE_DIR:MarCmd>
    )
elseif (UNIX)
  add_custom_command(
    TARGET MarCmd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../bin/$<CONFIG>/vendor/PluS/PluS/PluS.so"
    $<TARGET_FILE_DIR:MarCmd>
    )
endif()
