#pragma once

#include <set>

#include "ModuleInfo.h"

namespace MarC
{
	struct ExecutableInfo;

	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;

	struct ExecutableInfo
	{
		std::map<std::string, uint64_t> moduleNameMap;
		std::vector<ModuleInfoRef> modules;
		std::set<Symbol> symbols;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
	public:
		static ExecutableInfoRef loadFromFile(const std::string& path); // TODO: Implement
	};
}