cmake_minimum_required(VERSION 3.8)

project("MarCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	MarCore STATIC
	"src/MarCore.cpp"
	"src/Memory.cpp"
	
	
	"src/Interpreter.cpp"
 "include/Linker.h" "include/LinkerOutput.h" "src/Linker.cpp" "src/LinkerOutput.cpp" "include/ConvertInPlace.h" "include/Compiler.h" "include/CompilerTypes.h" "include/BytecodeTypes.h" "src/BytecodeTypes.cpp" "src/CompilerTypes.cpp" "include/AsmInstructions.h" "src/Compiler.cpp" "include/AsmTokenizer.h" "include/AsmTokenizerTypes.h" "src/AsmTokenizer.cpp" "src/AsmInstructions.cpp"  "include/ModuleLocator.h" "src/ModuleLocator.cpp")

target_include_directories(
	MarCore PUBLIC
	"include"
)