#pragma once

#include "AssemblerOutput.h"

namespace MarC
{
	struct ExecutableInfo
	{
		MemoryRef staticStack;
		std::vector<ModuleInfoRef> modules;
	};

	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;
}