cmake_minimum_required(VERSION 3.8)

project("MarCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	MarCore STATIC
	"src/MarCore.cpp"
	"src/Memory.cpp"
	"src/Linker.cpp"
	"src/ModuleInfo.cpp"
	"src/Assembler.cpp"
	"src/AsmTokenizer.cpp"
	"src/AsmInstructions.cpp"
	"src/VirtualAsmTokenList.cpp"
	"src/ExecutableInfo.cpp"
	"src/Disassembler.cpp"
	"src/ModulePack.cpp"
	"src/types/DisAsmTypes.cpp"
	"src/types/AsmTokenizerTypes.cpp"
	"src/types/BytecodeTypes.cpp"
	"src/types/AssemblerTypes.cpp"
	"src/runtime/Interpreter.cpp"
	"src/fileio/ModuleLocator.cpp"
	"src/fileio/ExtensionLocator.cpp"
	"src/fileio/ExecutableLoader.cpp"
	"src/fileio/CodeFileReader.cpp"
	"src/fileio/ModuleLoader.cpp"
)

target_include_directories(
	MarCore PUBLIC
	"include"
	"../vendor/PluS/PluS/include"
)

SET_TARGET_PROPERTIES(
	MarCore PROPERTIES PREFIX ""
)

target_link_libraries(
	MarCore PUBLIC 
	PluS
)

target_compile_definitions(
	MarCore PUBLIC 
	${PLUS_COMPILE_DEFINITIONS}
)

if (MSVC) 
	target_link_options(MarCore PRIVATE $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
endif()
