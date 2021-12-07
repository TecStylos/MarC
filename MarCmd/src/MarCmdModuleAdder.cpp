#include "MarCmdModuleAdder.h"

#include <fstream>

#include <MarCore.h>

namespace MarCmd
{
	std::string modNameFromPath(const std::string& filepath)
	{
		return std::filesystem::path(filepath).stem().string();
	}

	std::string readFile(const std::string& filepath)
	{
		std::ifstream f(filepath);
		if (!f.good())
			return "";

		std::string result;
		while (!f.eof())
		{
			std::string line;
			std::getline(f, line);
			result.append(line);
			result.push_back('\n');
		}
		if (!result.empty())
			result.pop_back();

		return result;
	}
	
	void addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, void* pParam)
	{
		auto extension = std::filesystem::path(modPath).extension().string();

		if (extension == ".mcc")
			throw std::runtime_error("Building of *.mcc files is not supported at the moment!");
		if (extension != ".mca")
			throw std::runtime_error("The input file is not supported!");

		bool verbose = *(bool*)pParam;
		std::string codeStr = readFile(modPath);
		MarC::AsmTokenizer tokenizer(codeStr);
		/*MarC::Assembler assembler(tokenizer.getTokenList(), modName);

		if (verbose)
			std::cout << "Tokenizing module '" << modName << "'..." << std::endl;
		if (!tokenizer.tokenize())
			throw tokenizer.lastError();

		if (verbose)
			std::cout << "Assembling module '" << modName << "'..." << std::endl;
		if (!assembler.assemble())
			throw assembler.lastError();

		if (verbose)
			std::cout << "Adding module '" << assembler.getModuleInfo()->moduleName << "' to the linker..." << std::endl;
		if (!linker.addModule(assembler.getModuleInfo()))
			throw linker.lastError();*/
	}
}