#include "MarCmdInterpreter.h"

#include <iostream>
#include <fstream>

#include "MarCore.h"
#include "PermissionGrantPrompt.h"
#include "MarCmdModuleAdder.h"

namespace MarCmd
{
	int Interpreter::run(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		MarC::ExecutableInfoRef exeInfo;
		std::string inMod = modNameFromPath(settings.inFile);
		auto extension = std::filesystem::path(settings.inFile).extension().string();

		if (extension == ".mcc")
			throw std::runtime_error("Interpreting *.mcc files is not supported at the moment!");
		else if (extension == ".mce")
		{
			
			std::ifstream iStream(settings.inFile, std::ios::binary | std::ios::in);
			if (!iStream.is_open())
			{
				std::cout << "Unable to open input file!" << std::endl;
				return -1;
			}
			exeInfo = MarC::ExecutableInfo::create();

			if (verbose)
				std::cout << "Loading input file from disk..." << std::endl;
			MarC::deserialize(*exeInfo, iStream);

			if (!iStream.good())
			{
				std::cout << "An error occured while reading the application from disk!" << std::endl;
				return -1;
			}
		}
		else if (extension == ".mca")
		{
			MarC::Linker linker;

			try
			{
				addModule(linker, settings.inFile, inMod, &verbose);
			}
			catch (const MarC::AsmTokenizerError& err)
			{
				std::cout << "An error occured while running the tokenizer!:" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return -1;\
			}
			catch (const MarC::AssemblerError& err)
			{
				std::cout << "An error occured while runing the assembler!" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return -1;
			}
			catch (const MarC::LinkerError& err)
			{
				std::cout << "An error occured while running the linker!:" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return -1;
			}
			catch (const std::runtime_error& err)
			{
				std::cout << "An unexpected error occured!:" << std::endl
					<< "  " << err.what() << std::endl;
				return -1;
			}

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

			exeInfo = linker.getExeInfo();
		}
		else
		{
			std::cout << "Unsupported input file!" << std::endl;
			return -1;
		}

		MarC::Interpreter interpreter(exeInfo);
		for (auto& entry : settings.extDirs)
			interpreter.addExtDir(entry);

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

		Timer timer;
		if (verbose)
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

		if (verbose)
			std::cout << "  Reason: '" << interpreter.lastError().getCodeStr() << "'" << std::endl;

		if (verbose)
			std::cout << "Executed " << interpreter.nInsExecuted() << " instructions in " << timer.microseconds() << " microseconds" << std::endl;

		return (int)exitCode;
	}
}
