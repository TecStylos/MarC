#include <filesystem>
#include <iostream>
#include <set>

#include "CmdArgParser.h"

#include "MarCmdHelp.h"
#include "MarCmdSettings.h"
#include "MarCmdInterpreter.h"
#include "MarCmdLiveAsmInterpreter.h"

int main(int argc, const char** argv, const char** env)
{
	std::map<std::string, std::string> envMap;
	for (int i = 0; env[i] != 0; ++i)
	{
		std::string envStr = env[i];
		auto sep = envStr.find('=');
		std::string key = envStr.substr(0, sep);
		std::string val = envStr.substr(sep + 1);
		envMap.insert({ key, val });
	}

	using Mode = MarCmd::Mode;

	MarCmd::Settings settings;

	MarCmd::CmdArgParser cmd(argc, argv);

	while (cmd.hasNext())
	{
		std::string elem = cmd.getNext();

		if (elem == "--help")
		{
			settings.mode = Mode::Help;
			continue;
		}
		if (elem == "--grantall")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::GrantAll);
			continue;
		}
		if (elem == "--verbose")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::Verbose);
			continue;
		}
		if (elem == "--livecode")
		{
			settings.mode = Mode::LiveCode;
			continue;
		}
		if (elem == "--liveasm")
		{
			settings.mode = Mode::LiveAsm;
			continue;
		}
		if (elem == "--build")
		{
			settings.mode = Mode::Build;
			continue;
		}
		if (elem == "--interpret")
		{
			settings.mode = Mode::Interpret;
			continue;
		}
		if (elem == "-o")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing output file!" << std::endl;
				return -1;
			}
			settings.outFile = cmd.getNext();
			continue;
		}
		if (elem == "-m")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing module directory!" << std::endl;
				return -1;
			}
			settings.modDirs.insert(cmd.getNext());
			continue;
		}
		if (elem == "-e")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing extension directory!" << std::endl;
				return -1;
			}
			settings.extDirs.insert(cmd.getNext());
			continue;
		}
		if (elem == "--keeponexit")
		{
			settings.exitBehavior = MarCmd::ExitBehavior::KeepOnExit;
			continue;
		}
		if (elem == "--closeonexit")
		{
			settings.exitBehavior = MarCmd::ExitBehavior::CloseOnExit;
			continue;
		}
		if (elem == "--debug")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::Debug);
			continue;
		}
		if (elem == "--profile")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::Profile);
			continue;
		}

		settings.inFile = elem;
		settings.modDirs.insert(std::filesystem::path(settings.inFile).parent_path().string());
	}

	if (settings.mode == Mode::None)
	{
		if (settings.inFile.empty())
		{
			std::cout <<
				"MarCmd" << std::endl <<
				"  No mode selected!" << std::endl <<
				"  For help type 'MarCmd --help'" << std::endl;
			return -1;
		}

		settings.mode = Mode::Interpret;
	}

	if (settings.modDirs.empty())
		settings.modDirs.insert(std::filesystem::current_path().string());
	if (settings.extDirs.empty())
		settings.extDirs.insert(std::filesystem::current_path().string());

	int exitCode = 0;

	{
		switch (settings.mode)
		{
		case Mode::Help:
			std::cout << MarCmd::HelpText << std::endl;
			exitCode = 0;
			break;
		case Mode::LiveCode:
			std::cout << "Mode 'livecode' has not been implemented yet!" << std::endl;
			exitCode = -1;
			break;
		case Mode::LiveAsm:
			exitCode = MarCmd::LiveAsmInterpreter::run(settings);
			break;
		case Mode::Build:
			std::cout << "Mode 'link' has not been implemented yet!" << std::endl;
			exitCode = -1;
			break;
		case Mode::Interpret:
			exitCode = MarCmd::Interpreter::run(settings);
			break;
		}
	}

	if (
		settings.exitBehavior == MarCmd::ExitBehavior::KeepOnExit ||
		(
			settings.exitBehavior == MarCmd::ExitBehavior::CloseWhenZero &&
			exitCode != 0
			)
		)
	{
		std::cout << "Press enter to exit...";
		std::cin.clear();
		std::cin.get();
	}

	return exitCode;
}