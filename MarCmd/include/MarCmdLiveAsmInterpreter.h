#pragma once

#include <set>
#include <string>

#include "MarCmdFlags.h"

namespace MarCmd
{
	class LiveAsmInterpreter
	{
	public:
		static int run(const std::set<std::string>& modDirs, Flags<CmdFlags> flags);
	private:
		static std::string readCodeFromConsole();
	};
}