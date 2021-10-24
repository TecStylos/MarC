#include <iostream>
#include <set>

#include "CmdArgParser.h"
#include "MarCmdMode.h"

#include "MarCmdInterpreter.h"

bool startswith(const std::string& left, const std::string& right)
{
	return left.find(right) == 0;
}

int main(int argc, const char** argv, const char** env)
{
	using Mode = MarCmd::Mode;

	bool verbose = false;
	Mode mode = Mode::None;
	std::string inFile = "";
	std::string outFile = "";
	std::set<std::string> modDirs = { "./" };
	std::string runFile = "";

	MarCmd::CmdArgParser cmd(argc, argv);

	while (cmd.hasNext())
	{
		std::string elem = cmd.getNext();

		if (startswith(elem, "--help"))
		{
			mode = Mode::Help;
			continue;
		}
		if (startswith(elem, "--verbose"))
		{
			verbose = true;
			continue;
		}
		if (startswith(elem, "--livecode"))
		{
			mode = Mode::LiveCode;
			continue;
		}
		if (startswith(elem, "--liveasm"))
		{
			mode = Mode::LiveAsm;
			continue;
		}
		if (startswith(elem, "--execute"))
		{
			mode = Mode::Execute;
			continue;
		}
		if (startswith(elem, "--assemble"))
		{
			mode = Mode::Assemble;
			continue;
		}
		if (startswith(elem, "--compile"))
		{
			mode = Mode::Compile;
			continue;
		}
		if (startswith(elem, "--link"))
		{
			mode = Mode::Link;
			continue;
		}
		if (startswith(elem, "-i"))
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing input file!" << std::endl;
				return -1;
			}
			inFile = cmd.getNext();
			continue;
		}
		if (startswith(elem, "-o"))
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing output file!" << std::endl;
				return -1;
			}
			outFile = cmd.getNext();
			continue;
		}
		if (startswith(elem, "--mod"))
		{
			if (!cmd.hasNext())
			{
				std::cout << "Missing module directory!" << std::endl;
				return -1;
			}
			modDirs.insert(cmd.getNext());
			continue;
		}

		runFile = elem;
		mode = Mode::Interpret;
		break;
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
		std::cout << "Mode 'help' has not been implemented yet!" << std::endl;
		exitCode = -1;
		break;
	case Mode::LiveCode:
		std::cout << "Mode 'livecode' has not been implemented yet!" << std::endl;
		exitCode = -1;
		break;
	case Mode::LiveAsm:
		std::cout << "Mode 'liveasm' has not been implemented yet!" << std::endl;
		exitCode = -1;
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
		exitCode = MarCmd::Interpreter::run(runFile, modDirs, verbose);
		break;
	}

	if (exitCode != 0)
	{
		std::cout << "Press enter to exit...";
		std::cin.clear();
		std::cin.get();
	}

	return exitCode;
}