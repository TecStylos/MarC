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
	"src/LinkerOutput.cpp"
	"src/BytecodeTypes.cpp"
	"src/CompilerTypes.cpp"
	"src/Compiler.cpp"
	"src/AsmTokenizer.cpp"
	"src/AsmInstructions.cpp"
	"src/ModuleLocator.cpp"
	"src/VirtualAsmTokenList.cpp"
	"src/AsmTokenizerTypes.cpp"
)

target_include_directories(
	MarCore PUBLIC
	"include"
)

if (MSVC) 
  target_link_options(MarCore PRIVATE $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
endif()