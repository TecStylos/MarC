#include "MarCmdInterpreter.h"

#include <iostream>
#include <fstream>

#include "MarCore.h"
#include "PermissionGrantPrompt.h"

namespace MarCmd
{
	int Interpreter::run(const Settings& settings)
	{
		std::string inMod = modNameFromPath(settings.inFile);

		Timer timer;

		MarC::Linker linker;
		MarC::Interpreter interpreter(linker.getExeInfo());

		for (auto& entry : settings.extDirs)
			interpreter.addExtDir(entry);

		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);
		if (!addModule(linker, settings.inFile, inMod, &verbose))
			return -1;

		if (!linker.autoAddMissingModules(settings.modDirs, &addModule, &verbose))
		{
			std::cout << "An error occured while auto adding the required modules!:" << std::endl
				<< "  " << linker.lastError().getMessage() << std::endl;
			return -1;
		}

		if (settings.flags.hasFlag(CmdFlags::Verbose))
			std::cout << "Linking the application..." << std::endl;
		if (!linker.link())
		{
			std::cout << "An error occured while running the linker!" << std::endl
				<< "  " << linker.lastError().getMessage() << std::endl;
			return -1;
		}

		if (interpreter.hasUngrantedPerms())
		{
			if (settings.flags.hasFlag(CmdFlags::GrantAll))
			{
				interpreter.grantAllPerms();
			}
			else
			{
				std::set<std::string> toGrant;

				auto manPerms = interpreter.getUngrantedPerms(interpreter.getManPerms());
				if (!manPerms.empty())
				{
					std::cout << "  <!> A module requests the following MANDATORY permissions: " << std::endl;
					permissionGrantPrompt(manPerms, toGrant);
				}

				auto optPerms = interpreter.getUngrantedPerms(interpreter.getOptPerms());
				if (!optPerms.empty())
				{
					std::cout << "  <!> A module requests the following OPTIONAL permissions: " << std::endl;
					permissionGrantPrompt(optPerms, toGrant);
				}

				interpreter.grantPerms(toGrant);
			}
		}

		if (settings.flags.hasFlag(CmdFlags::Verbose))
			std::cout << "Starting interpreter..." << std::endl;
		timer.start();
		bool intResult = interpreter.interpret();
		timer.stop();
		if (!intResult && !interpreter.lastError().isOK())
		{
			std::cout << std::endl << "An error occured while interpreting the code!" << std::endl
				<< "    " << interpreter.lastError().getMessage() << std::endl;
			return -1;
		}

		int64_t exitCode = interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		std::cout << std::endl << "Module '" << inMod << "' exited with code " << exitCode << "." << std::endl;

		if (settings.flags.hasFlag(CmdFlags::Verbose))
			std::cout << "  Reason: '" << interpreter.lastError().getCodeStr() << "'" << std::endl;

		if (settings.flags.hasFlag(CmdFlags::Verbose))
			std::cout << "Executed " << interpreter.nInsExecuted() << " instructions in " << timer.microseconds() << " microseconds" << std::endl;

		return (int)exitCode;
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

	bool Interpreter::addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, void* pParam)
	{
		bool verbose = *(bool*)pParam;
		std::string codeStr = readFile(modPath);
		MarC::AsmTokenizer tokenizer(codeStr);
		MarC::Assembler assembler(tokenizer.getTokenList(), modName);

		if (verbose)
			std::cout << "Tokenizing module '" << modName << "'..." << std::endl;
		if (!tokenizer.tokenize())
		{
			std::cout << "An error occured while running the tokenizer!" << std::endl
				<< "  " << tokenizer.lastError().getMessage() << std::endl;
			return false;
		}

		if (verbose)
			std::cout << "Assembling module '" << modName << "'..." << std::endl;
		if (!assembler.assemble())
		{
			std::cout << "An error occured while running the assembler!:" << std::endl
				<< "  " << assembler.lastError().getMessage() << std::endl;
			return false;
		}

		if (verbose)
			std::cout << "Adding module '" << assembler.getModuleInfo()->moduleName << "' to the linker..." << std::endl;
		if (!linker.addModule(assembler.getModuleInfo()))
		{
			std::cout << "An error occurd while adding the module '" << assembler.getModuleInfo()->moduleName << "' to the linker!:" << std::endl
				<< "  " << linker.lastError().getMessage() << std::endl;
			return false;
		}

		return true;
	}
}
