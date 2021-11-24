#pragma once

#include <set>
#include <string>

#include "MarCmdMode.h"
#include "MarCmdFlags.h"

namespace MarCmd
{
	enum class ExitBehavior
	{
		CloseWhenZero,
		CloseOnExit,
		KeepOnExit,
	};

	struct Settings
	{
		Flags<MarCmd::CmdFlags> flags;
		Mode mode = Mode::None;
		std::string inFile = "";
		std::string outFile = "";
		std::set<std::string> modDirs;
		std::set<std::string> extDirs;
		ExitBehavior exitBehavior = ExitBehavior::CloseWhenZero;
	};
}