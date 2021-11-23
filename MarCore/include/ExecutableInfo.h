#pragma once

#include <set>
#include <iostream>

#include "ModuleInfo.h"

namespace MarC
{
	struct ExecutableInfo;

	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;

	struct ExecutableInfo
	{
		std::set<Symbol> symbols;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
		std::vector<ModuleInfoRef> modules;
		std::map<std::string, uint64_t> moduleNameMap;
	};

	std::ostream& operator<<(std::ostream& outStream, const ExecutableInfo& eInfo);
	std::istream& operator>>(std::istream& inStream, ExecutableInfo& eInfo);
}