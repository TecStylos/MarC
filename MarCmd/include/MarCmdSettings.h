#pragma once

#include <set>
#include <string>

#include <MarCore.h>

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
		MarC::Flags<MarCmd::CmdFlags> flags;
		Mode mode = Mode::None;
		std::string inFile = "";
		std::string outFile = "";
		std::string exeDir = "";
		std::set<std::string> modDirs;
		std::set<std::string> extDirs;
		ExitBehavior exitBehavior = ExitBehavior::CloseWhenZero;
	};
}