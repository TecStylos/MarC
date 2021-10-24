#include "MarCmdInterpreter.h"

#include <iostream>
#include <fstream>

#include "MarCore.h"

namespace MarCmd
{
	int Interpreter::run(const std::string& inFile, const std::set<std::string>& modDirs, bool verbose)
	{
		std::string inMod = modNameFromPath(inFile);

		Timer timer;

		MarC::Linker linker;
		MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

		if (!addModule(linker, inFile, inMod, verbose))
			return -1;

		while (linker.hasMissingModules())
		{
			auto& misMods = linker.getMissingModules();
			auto modPaths = MarC::locateModules(modDirs, misMods);

			for (auto& pair : modPaths)
			{
				if (pair.second.empty())
				{
					std::cout << "Unable to find module '" << pair.first << "'!" << std::endl;
					return -1;
				}
				if (pair.second.size() > 1)
				{
					std::cout << "Module '" << pair.first << "' is ambigious! Found " << pair.second.size() << " matching files!" << std::endl;
					for (auto& p : pair.second)
						std::cout << "  " << p << std::endl;
					return -1;
				}

				if (!addModule(linker, pair.second[0], pair.first, verbose))
					return -1;
			}
		}

		std::cout << "Linking the application...";
		if (linker.link())
			std::cout << " DONE" << std::endl;
		else
		{
			std::cout << "  ERROR" << std::endl;
			return -1;
		}

		std::cout << "Running interpreter...";

		timer.start();
		bool intResult = interpreter.interpret();
		timer.stop();
		if (intResult || interpreter.lastError().isOK())
			std::cout << " DONE" << std::endl;
		else
		{
			std::cout << std::endl << "  An error occured while interpreting the code!" << std::endl
				<< "    " << interpreter.lastError().getMessage() << std::endl;
			return -1;
		}

		std::cout << "Executed " << interpreter.nInsExecuted() << " instructions in " << timer.microseconds() << " microseconds" << std::endl;

		int exitCode = interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		std::cout << "Exit code: " << exitCode << std::endl;

		return exitCode;
	}

	std::string Interpreter::modNameFromPath(const std::string& filepath)
	{
		return std::filesystem::path(filepath).stem().string();
	}

	std::string Interpreter::readFile(const std::string& filepath)
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

	bool Interpreter::addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, bool verbose)
	{
		MarC::AsmTokenizer tokenizer(readFile(modPath));
		MarC::Compiler compiler(tokenizer.getTokenList(), modName);

		std::cout << "Tokenizing '" << modPath << "'...";
		if (tokenizer.tokenize())
			std::cout << " DONE" << std::endl;
		else
		{
			std::cout << "  ERROR" << std::endl
				<< "    " << tokenizer.lastError().getMessage() << std::endl;
			return false;
		}

		std::cout << "Compiling '" << modPath << "'...";
		if (compiler.compile())
			std::cout << " DONE" << std::endl;
		else
		{
			std::cout << "  ERROR" << std::endl
				<< "    " << compiler.lastError().getMessage() << std::endl;
			return false;
		}

		std::cout << "Adding module '" << modName << "' to linker...";
		if (linker.addModule(compiler.getModuleInfo()))
			std::cout << " DONE" << std::endl;
		else
		{
			std::cout << "  ERROR" << std::endl;
			return false;
		}

		return true;
	}
}