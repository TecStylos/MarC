#pragma once

#include "MarCmdSettings.h"

#include "ConsoleGetChar.h"

namespace MarCmd
{
	class Debugger
	{
	public:
		static int run(const Settings& settings);
	};
}