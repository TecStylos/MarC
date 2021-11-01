#include <filesystem>
#include <iostream>
#include <set>

#include "CmdArgParser.h"
#include "MarCmdMode.h"

#include "MarCmdHelp.h"
#include "MarCmdFlags.h"
#include "MarCmdInterpreter.h"
#include "MarCmdLiveAsmInterpreter.h"

enum class ExitBehavior
{
	CloseWhenZero,
	CloseOnExit,
	KeepOnExit,
};

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

	MarCmd::Flags<MarCmd::CmdFlags> flags;
	Mode mode = Mode::None;
	std::string inFile = "";
	std::string outFile = "";
	std::set<std::string> modDirs = {};
	std::string runFile = "";
	ExitBehavior exitBehavior = ExitBehavior::CloseWhenZero;

	MarCmd::CmdArgParser cmd(argc, argv);

	while (cmd.hasNext())
	{
		std::string elem = cmd.getNext();

		if (elem == "--help")
		{
			mode = Mode::Help;
			continue;
		}
		if (elem == "--verbose")
		{
			flags.setFlag(MarCmd::CmdFlags::Verbose);
			continue;
		}
		if (elem == "--livecode")
		{
			mode = Mode::LiveCode;
			continue;
		}
		if (elem == "--liveasm")
		{
			mode = Mode::LiveAsm;
			continue;
		}
		if (elem == "--execute")
		{
			mode = Mode::Execute;
			continue;
		}
		if (elem == "--assemble")
		{
			mode = Mode::Assemble;
			continue;
		}
		if (elem == "--compile")
		{
			mode = Mode::Compile;
			continue;
		}
		if (elem == "--link")
		{
			mode = Mode::Link;
			continue;
		}
		if (elem == "-i")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing input file!" << std::endl;
				return -1;
			}
			inFile = cmd.getNext();
			continue;
		}
		if (elem == "-o")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing output file!" << std::endl;
				return -1;
			}
			outFile = cmd.getNext();
			continue;
		}
		if (elem == "-m")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing module directory!" << std::endl;
				return -1;
			}
			modDirs.insert(cmd.getNext());
			continue;
		}
		if (elem == "--keeponexit")
		{
			exitBehavior = ExitBehavior::KeepOnExit;
			continue;
		}
		if (elem == "--closeonexit")
		{
			exitBehavior = ExitBehavior::CloseOnExit;
			continue;
		}
		if (elem == "--debug")
		{
			flags.setFlag(MarCmd::CmdFlags::Debug);
			continue;
		}
		if (elem == "--profile")
		{
			flags.setFlag(MarCmd::CmdFlags::Profile);
			continue;
		}

		runFile = elem;
		modDirs.insert(std::filesystem::path(runFile).parent_path().string());
		mode = Mode::Interpret;
	}

	if (mode == Mode::None)
	{
		std::cout <<
			"MarCmd" << std::endl <<
			"  No mode selected!" << std::endl <<
			"  For help type 'MarCmd --help'" << std::endl;
		return -1;
	}

	int exitCode = 0;

	switch (mode)
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
		exitCode = MarCmd::LiveAsmInterpreter::run(modDirs, flags);
		break;
	case Mode::Execute:
		std::cout << "Mode 'execute' has not been implemented yet!" << std::endl;
		exitCode = -1;
		break;
	case Mode::Assemble:
		std::cout << "Mode 'assemble' has not been implemented yet!" << std::endl;
		exitCode = -1;
		break;
	case Mode::Compile:
		std::cout << "Mode 'compile' has not been implemented yet!" << std::endl;
		exitCode = -1;
		break;
	case Mode::Link:
		std::cout << "Mode 'link' has not been implemented yet!" << std::endl;
		exitCode =  -1;
		break;
	case Mode::Interpret:
		exitCode = MarCmd::Interpreter::run(runFile, modDirs, flags);
		break;
	}

	if (
		exitBehavior == ExitBehavior::KeepOnExit ||
		(
			exitBehavior == ExitBehavior::CloseWhenZero && exitCode != 0
			)
		)
	{
		std::cout << "Press enter to exit...";
		std::cin.clear();
		std::cin.get();
	}

	return exitCode;
}
