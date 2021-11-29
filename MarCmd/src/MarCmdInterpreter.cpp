#include "MarCmdInterpreter.h"

#include <iostream>
#include <fstream>

#include "MarCore.h"
#include "PermissionGrantPrompt.h"
#include "MarCmdExeInfoLoader.h"

namespace MarCmd
{
	int Interpreter::run(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		auto exeInfo = loadExeInfo(settings);
		if (!exeInfo)
			return -1;

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
					permissionGrantPrompt(PermissionPromptType::Mandatory, manPerms, toGrant);

				auto optPerms = interpreter.getUngrantedPerms(interpreter.getOptPerms());
				if (!optPerms.empty())
					permissionGrantPrompt(PermissionPromptType::Optional, optPerms, toGrant);

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

		if (!settings.flags.hasFlag(CmdFlags::NoExitInfo))
		{
			std::cout << std::endl << "Module '" << exeInfo->modules[0] << "' exited with code " << exitCode << "." << std::endl;

			if (verbose)
				std::cout << "  Reason: '" << interpreter.lastError().getCodeStr() << "'" << std::endl;

			if (verbose)
				std::cout << "Executed " << interpreter.nInsExecuted() << " instructions in " << timer.microseconds() << " microseconds" << std::endl;
		}

		return (int)exitCode;
	}
}
