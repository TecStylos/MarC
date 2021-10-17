#pragma once

#include "CompilerTypes.h"

namespace MarC
{
	struct ExecutableInfo
	{
		ExecutableInfo();
	public:
		std::map<std::string, uint64_t> moduleNameMap;
		std::vector<ModuleInfoRef> modules;
	};

	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;
}