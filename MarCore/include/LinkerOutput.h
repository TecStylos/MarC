#pragma once

#include <set>

#include "CompilerTypes.h"

namespace MarC
{
	struct ExecutableInfo
	{
		ExecutableInfo();
	public:
		std::map<std::string, uint64_t> moduleNameMap;
		std::vector<ModuleInfoRef> modules;
		std::set<Symbol> symbols;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
	};

	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;
}