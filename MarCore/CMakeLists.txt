cmake_minimum_required(VERSION 3.8)

project("MarCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	MarCore STATIC
	"src/MarCore.cpp"
	"src/Memory.cpp"
	"src/Interpreter.cpp"
	"src/Linker.cpp"
	"src/ModuleInfo.cpp"
	"src/BytecodeTypes.cpp"
	"src/AssemblerTypes.cpp"
	"src/Assembler.cpp"
	"src/AsmTokenizer.cpp"
	"src/AsmInstructions.cpp"
	"src/ModuleLocator.cpp"
	"src/VirtualAsmTokenList.cpp"
	"src/AsmTokenizerTypes.cpp"
	"src/ExtensionLocator.cpp"
	"src/ExecutableInfo.cpp"
	"src/Disassembler.cpp"
	"src/DisAsmTypes.cpp"
	"src/ModuleLoader.cpp"
	"src/CodeFileReader.cpp"
	"src/ModulePack.cpp"
	"src/ExecutableLoader.cpp"
 "include/errors/AsmTokenizerError.h" "include/errors/InterpreterError.h" "include/errors/MacroExpansionError.h" "include/Macro.h")

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
