#include <filesystem>
#include <iostream>
#include <set>

#include "CmdArgParser.h"

#include "MarCmdFlags.h"
#include "MarCmdHelp.h"
#include "MarCmdBuilder.h"
#include "MarCmdDisassembler.h"
#include "MarCmdInterpreter.h"
#include "Debugger/Debugger.h"
#include "MarCmdLiveAsmInterpreter.h"
#include "CurrExePath.h"

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
	settings.exeDir = std::filesystem::path(MarCmd::CurrExePath()).parent_path().string();
	settings.modDirs.insert(std::filesystem::current_path().string());
	settings.modDirs.insert(settings.exeDir);
	settings.extDirs.insert(std::filesystem::current_path().string());
	settings.extDirs.insert(settings.exeDir);

	MarCmd::CmdArgParser cmd(argc, argv);

	while (cmd.hasNext())
	{
		std::string elem = cmd.getNext();

		if (elem == "--help")
		{
			settings.mode = Mode::Help;
		}
		else if (elem == "--grantall")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::GrantAll);
		}
		else if (elem == "--verbose")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::Verbose);
		}
		else if (elem == "--livecode")
		{
			settings.mode = Mode::LiveCode;
		}
		else if (elem == "--liveasm")
		{
			settings.mode = Mode::LiveAsm;
		}
		else if (elem == "--build")
		{
			settings.mode = Mode::Build;
		}
		else if (elem == "--disasm")
		{
			settings.mode = Mode::Disassemble;
		}
		else if (elem == "--debug")
		{
			settings.mode = Mode::Debug;
		}
		else if (elem == "--interpret")
		{
			settings.mode = Mode::Interpret;
		}
		else if (elem == "-o")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing output file!" << std::endl;
				return -1;
			}
			settings.outFile = cmd.getNext();
		}
		else if (elem == "-m")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing module directory!" << std::endl;
				return -1;
			}
			settings.modDirs.insert(cmd.getNext());
		}
		else if (elem == "-e")
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing extension directory!" << std::endl;
				return -1;
			}
			settings.extDirs.insert(cmd.getNext());
		}
		else if (elem == "--keeponexit")
		{
			settings.exitBehavior = MarCmd::ExitBehavior::KeepOnExit;
		}
		else if (elem == "--closeonexit")
		{
			settings.exitBehavior = MarCmd::ExitBehavior::CloseOnExit;
		}
		else if (elem == "--noexitinfo")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::NoExitInfo);
		}
		else if (elem == "--dbginfo")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::DebugInfo);
		}
		else if (elem == "--forcerefresh")
		{
		  settings.flags.setFlag(MarCmd::CmdFlags::ForceRefresh);
		}
		else if (elem == "--profile")
		{
			settings.flags.setFlag(MarCmd::CmdFlags::Profile);
		}
		else
		{
			settings.inFile = elem;
			settings.modDirs.insert(std::filesystem::path(settings.inFile).parent_path().string());
		}
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

		settings.flags.setFlag(MarCmd::CmdFlags::NoExitInfo);
		settings.mode = Mode::Interpret;
	}

	if (settings.flags.hasFlag(MarCmd::CmdFlags::Verbose))
		settings.flags.clrFlag(MarCmd::CmdFlags::NoExitInfo);

	int exitCode = 0;

	try
	{
		switch (settings.mode)
		{
		case Mode::None: // Cannot be Mode::None, see previous if branch
			break;
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
			exitCode = MarCmd::Builder::run(settings);
			break;
		case Mode::Disassemble:
			exitCode = MarCmd::Disassembler::run(settings);
			break;
		case Mode::Debug:
			exitCode = MarCmd::Debugger::run(settings);
			break;
		case Mode::Interpret:
			exitCode = MarCmd::Interpreter::run(settings);
			break;
		}
	}
	catch (const MarC::MarCoreError& err)
	{
		std::cout << "ERROR: " << err.what() << std::endl;
		exitCode = -1;
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
		std::string temp;
		std::getline(std::cin, temp);
	}

	return exitCode;
}
