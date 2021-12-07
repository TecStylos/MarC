#pragma once

#include "ExecutableInfo.h"

namespace MarC
{
	class ExecutableLoader
	{
	public:
		static MarC::ExecutableInfoRef load(const std::string& exePath);
	};
}