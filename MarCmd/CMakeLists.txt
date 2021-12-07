cmake_minimum_required(VERSION 3.8)

project("MarCmd")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	MarCmd
	"src/EntryPoint.cpp"
	"src/MarCmdInterpreter.cpp"
	"src/MarCmdLiveAsmInterpreter.cpp"
    "src/PermissionGrantPrompt.cpp"
    "src/MarCmdBuilder.cpp"
    
    "src/MarCmdDisassembler.cpp"
    "src/Debugger/Debugger.cpp"
    "src/Debugger/ConsoleWindow.cpp"
    "src/Debugger/ConsoleHelper.cpp"
    "src/Debugger/DebugWindow.cpp"
    
 "include/AutoExecutableLoader.h" "src/AutoExecutableLoader.cpp")

target_include_directories(
	MarCmd PUBLIC 
	"${CMAKE_SOURCE_DIR}/MarCmd/include"
	"${CMAKE_SOURCE_DIR}/MarCore/include"
)

set(
	MARCMD_LINK_LIBRARIES
	MarCore
)
set(
    MARCMD_COMPILE_DEFINITIONS
)
if (WIN32)
    set(
        MARCMD_COMPILE_DEFINITIONS
        ${MARCMD_COMPILE_DEFINITIONS}
        "MARCMD_PLATFORM_WINDOWS"
    )
elseif (UNIX)
    set(
        MARCMD_COMPILE_DEFINITIONS
        ${MARCMD_COMPILE_DEFINITIONS}
        "MARCMD_PLATFORM_UNIX"
    )
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(
	MARCMD_LINK_LIBRARIES
	${MARCMD_LINK_LIBRARIES}
	Threads::Threads
    )
endif()

target_compile_definitions(
    MarCmd PUBLIC
    ${MARCMD_COMPILE_DEFINITIONS}
)
target_link_libraries(
	MarCmd PUBLIC
	${MARCMD_LINK_LIBRARIES}
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
